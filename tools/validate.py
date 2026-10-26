#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = ["jsonschema>=4.23", "pyyaml>=6.0"]
# ///
"""Validate feature, evidence, library, and corpus data."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import defaultdict
from datetime import date
from pathlib import Path

import yaml
from jsonschema import Draft202012Validator

ROOT = Path(__file__).resolve().parents[1]
SCHEMA_DIR = ROOT / "schema"
FEATURE_DIR = ROOT / "data" / "features"
EVIDENCE_DIR = ROOT / "data" / "evidence"
LIBRARY_DIR = ROOT / "data" / "libraries"
CORPUS_FILE = ROOT / "data" / "corpus.yaml"
COMPILERS_FILE = ROOT / "data" / "compilers.yaml"
CORPUS_CACHE = ROOT / ".cache" / "corpus"

EXCERPT_LICENSES = {
    "MIT",
    "Apache-2.0",
    "BSD-2-Clause",
    "BSD-3-Clause",
    "BSL-1.0",
    "CC0-1.0",
}
MAX_EXCERPT_LINES = 15
PUBLISHED_LEVELS = {"manual", "verified"}
EXAMPLE_LINK = re.compile(r"\]\(\.\./\.\./examples/([^)\s]+\.cpp)\)")
TIER_RULES = {
    "A": {
        "min_evidence": 3,
        "require_example": True,
        "headings": [
            "解决什么问题",
            "基础用法",
            "进阶用法",
            "真实项目证据",
            "陷阱与边界",
            "库联动",
            "横向对比",
        ],
    },
    "B": {
        "min_evidence": 2,
        "require_example": False,
        "headings": [
            "解决什么问题",
            "基础用法",
            "真实项目证据",
            "陷阱与边界",
            "横向对比",
        ],
    },
    "C": {
        "min_evidence": 1,
        "require_example": False,
        "headings": [],
    },
}


def report_path(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


class Report:
    def __init__(self) -> None:
        self.errors: list[str] = []

    def error(self, path: Path, message: str) -> None:
        self.errors.append(f"{report_path(path)}: {message}")


def load_schema(name: str) -> dict:
    return json.loads((SCHEMA_DIR / name).read_text())


def validate_schema(report: Report, path: Path, data: object, schema: dict) -> bool:
    validator = Draft202012Validator(schema)
    errors = sorted(validator.iter_errors(data), key=lambda error: list(error.path))
    for error in errors:
        location = ".".join(str(part) for part in error.path) or "<root>"
        report.error(path, f"{location}: {error.message}")
    return not errors


def read_frontmatter(path: Path, report: Report) -> tuple[dict, str] | None:
    text = path.read_text(encoding="utf-8")
    if not text.startswith("---\n"):
        report.error(path, "missing YAML frontmatter")
        return None
    end = text.find("\n---\n", 4)
    if end == -1:
        report.error(path, "unterminated YAML frontmatter")
        return None
    try:
        data = yaml.safe_load(text[4:end])
    except yaml.YAMLError as exc:
        report.error(path, f"invalid YAML frontmatter: {exc}")
        return None
    if not isinstance(data, dict):
        report.error(path, "frontmatter must be a mapping")
        return None
    return data, text[end + 5 :].lstrip("\n")


def git(repo_dir: Path, *args: str) -> subprocess.CompletedProcess:
    return subprocess.run(["git", *args], cwd=repo_dir, text=True, capture_output=True)


def ensure_commit(repo_dir: Path, commit: str) -> bool:
    probe = git(repo_dir, "cat-file", "-e", f"{commit}^{{commit}}")
    if probe.returncode == 0:
        return True
    fetch = git(repo_dir, "fetch", "--depth", "1", "origin", commit)
    if fetch.returncode != 0:
        return False
    return git(repo_dir, "cat-file", "-e", f"{commit}^{{commit}}").returncode == 0


def check_corpus_link(
    report: Report,
    path: Path,
    evidence: dict,
    corpus_by_repo: dict[str, dict],
) -> None:
    repo = evidence["repo"]
    entry = corpus_by_repo.get(repo)
    if entry is None:
        report.error(path, f"repo {repo} is not declared in data/corpus.yaml")
        return
    if evidence["license"] != entry.get("license"):
        report.error(
            path,
            f"license {evidence['license']} differs from corpus declaration "
            f"{entry.get('license')}",
        )
    repo_dir = CORPUS_CACHE / repo.replace("/", "__")
    if not (repo_dir / ".git").exists():
        report.error(path, f"corpus cache missing: {repo_dir}")
        return
    if not ensure_commit(repo_dir, evidence["commit"]):
        report.error(
            path,
            f"commit {evidence['commit']} is not available in the cache; "
            f"run git -C {report_path(repo_dir)} fetch --depth 1 origin "
            f"{evidence['commit']}",
        )
        return
    object_spec = f"{evidence['commit']}:{evidence['path']}"
    exists = git(repo_dir, "cat-file", "-e", object_spec)
    if exists.returncode != 0:
        report.error(
            path, f"path not found at commit {evidence['commit']}: {evidence['path']}"
        )
        return
    start_text, end_text = evidence["lines"].split("-")
    start, end = int(start_text), int(end_text)
    if start > end:
        report.error(path, f"invalid line range: {evidence['lines']}")
        return
    shown = git(repo_dir, "show", object_spec)
    if shown.returncode != 0:
        report.error(path, f"cannot read {object_spec}")
        return
    source_lines = shown.stdout.splitlines()
    if end > len(source_lines):
        report.error(
            path, f"line range {evidence['lines']} exceeds {len(source_lines)} lines"
        )
        return
    if not evidence["excerpt"].strip():
        return
    expected = source_lines[start - 1 : end]
    actual = evidence["excerpt"].splitlines()
    if actual == expected:
        return
    for index, (found, wanted) in enumerate(zip(actual, expected), start=1):
        if found != wanted:
            report.error(
                path,
                f"excerpt line {index} differs from source: "
                f"expected {wanted!r}, found {found!r}",
            )
            return
    report.error(
        path,
        f"excerpt has {len(actual)} lines but line range "
        f"{evidence['lines']} has {len(expected)}",
    )


def check_standard(
    report: Report, path: Path, feature_id: str, standard: object
) -> None:
    if not isinstance(standard, str):
        return
    prefix = "cpp" + standard.removeprefix("C++")
    if not feature_id.startswith(prefix + "."):
        report.error(path, f"standard {standard} does not match id {feature_id}")


def check_date(report: Report, path: Path, field: str, value: object) -> None:
    if not isinstance(value, str):
        return
    try:
        date.fromisoformat(value)
    except ValueError:
        report.error(path, f"{field} is not a valid date: {value}")


def check_compilers(report: Report, features: dict[str, dict], table: object) -> None:
    if not isinstance(table, dict) or not isinstance(table.get("features"), dict):
        report.error(COMPILERS_FILE, "compiler table must contain a features mapping")
        return
    entries = table["features"]
    for feature_id, feature in features.items():
        entry = entries.get(feature_id)
        if not isinstance(entry, dict):
            report.error(COMPILERS_FILE, f"missing compiler entry for {feature_id}")
            continue
        compilers = feature.get("compilers")
        if isinstance(compilers, dict):
            for name in ("gcc", "clang", "msvc"):
                if compilers.get(name) != entry.get(name):
                    report.error(
                        COMPILERS_FILE,
                        f"{feature_id}: {name} {compilers.get(name)!r} differs from "
                        f"table {entry.get(name)!r}",
                    )
        standard = feature.get("standard")
        if isinstance(standard, str):
            expected = standard.removeprefix("C++")
            if not str(entry.get("source", "")).endswith("/" + expected):
                report.error(
                    COMPILERS_FILE,
                    f"{feature_id}: source does not point to the C++{expected} table",
                )
        check_date(
            report,
            COMPILERS_FILE,
            f"{feature_id}.checked_at",
            entry.get("checked_at"),
        )
    for feature_id in entries:
        if feature_id not in features:
            report.error(
                COMPILERS_FILE, f"compiler entry for unknown feature {feature_id}"
            )


def check_tier(
    report: Report,
    path: Path,
    feature: dict,
    body: str,
    feature_evidence: list[dict],
) -> None:
    tier = feature.get("tier")
    rule = TIER_RULES.get(tier)
    if rule is None:
        return
    published = [item for item in feature_evidence if item["level"] in PUBLISHED_LEVELS]
    if len(published) < rule["min_evidence"]:
        report.error(
            path,
            f"tier {tier} needs at least {rule['min_evidence']} published "
            f"evidence entries, found {len(published)}",
        )
    headings = {
        line[3:].strip() for line in body.splitlines() if line.startswith("## ")
    }
    missing = [heading for heading in rule["headings"] if heading not in headings]
    if missing:
        report.error(path, f"tier {tier} missing sections: {', '.join(missing)}")
    if not rule["require_example"]:
        return
    links = EXAMPLE_LINK.findall(body)
    if not links:
        report.error(path, "tier A needs a compilable example under examples/")
    for link in links:
        example = (path.parent / "../../examples" / link).resolve()
        if not example.is_file():
            report.error(path, f"example link does not exist: examples/{link}")


def check_evidence_mix(
    report: Report, path: Path, feature_evidence: list[dict]
) -> None:
    published = [item for item in feature_evidence if item["level"] in PUBLISHED_LEVELS]
    if published and not any(not item["is_test"] for item in published):
        report.error(
            path, "published evidence must include at least one non-test entry"
        )


def check_evidence_review(report: Report, path: Path, evidence: dict) -> None:
    level = evidence["level"]
    if level == "manual":
        if len(evidence["verified_by"]) < 1 or evidence["verified_at"] is None:
            report.error(path, "manual evidence needs a reviewer and verified_at")
        return
    if level == "verified":
        if len(evidence["verified_by"]) < 2 or evidence["verified_at"] is None:
            report.error(path, "verified evidence needs two reviewers and verified_at")
        if not evidence.get("submitted_by"):
            report.error(path, "verified evidence needs submitted_by")
        elif not any(
            reviewer != evidence["submitted_by"] for reviewer in evidence["verified_by"]
        ):
            report.error(
                path, "verified evidence needs at least one non-author reviewer"
            )
        return
    if level == "candidate" and evidence["verified_by"]:
        report.error(path, "candidate evidence must not list reviewers")


def check_generated_evidence(report: Report, path: Path, evidence: dict) -> None:
    generated = evidence["generated"]
    review = evidence.get("generated_review")
    if not generated:
        if review is not None:
            report.error(path, "generated_review requires generated: true")
        return
    if evidence["level"] not in PUBLISHED_LEVELS:
        return
    if not evidence.get("submitted_by"):
        report.error(path, "generated evidence needs submitted_by for exception review")
    if review is None:
        report.error(
            path,
            "generated code is rejected by default; add generated_review with an "
            "independent approver and rationale",
        )
        return
    if not review["rationale"].strip():
        report.error(path, "generated_review needs a rationale")
    if (
        evidence.get("submitted_by")
        and review["approved_by"] == evidence["submitted_by"]
    ):
        report.error(
            path, "generated_review needs an approver who is not the submitter"
        )


def check_claims(
    report: Report,
    path: Path,
    feature: dict,
    feature_evidence: list[dict],
) -> None:
    tier = feature.get("tier")
    claims = feature.get("claims")
    if tier == "A" and not claims:
        report.error(path, "tier A needs a claims ledger")
        return
    if not claims:
        return

    published = {
        item["id"]: item
        for item in feature_evidence
        if item["level"] in PUBLISHED_LEVELS
    }
    seen: set[str] = set()
    referenced: set[str] = set()
    for claim in claims:
        claim_id = claim["id"]
        if claim_id in seen:
            report.error(path, f"duplicate claim id {claim_id}")
        seen.add(claim_id)
        for evidence_id in claim["evidence"]:
            if evidence_id not in published:
                report.error(
                    path,
                    f"claim {claim_id} references {evidence_id}, which is not "
                    "published evidence for this feature",
                )
            referenced.add(evidence_id)
    orphans = sorted(set(published) - referenced)
    if orphans:
        report.error(
            path,
            "published evidence not referenced by any claim: " + ", ".join(orphans),
        )


def check_review(report: Report, path: Path, feature: dict) -> None:
    tier = feature.get("tier")
    review = feature.get("review")
    if tier == "A" and review is None:
        report.error(path, "tier A needs a review block")
        return
    if review is None:
        return
    author = review["author"]
    level = review["level"]
    reviewers = review["reviewers"]
    if level == "manual":
        if not reviewers:
            report.error(path, "manual review needs a reviewer")
        return
    if len(reviewers) < 2:
        report.error(path, "verified review needs two distinct reviewers")
    if not any(reviewer != author for reviewer in reviewers):
        report.error(path, "verified review needs at least one non-author reviewer")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--check-corpus",
        action="store_true",
        help="verify pinned commits, paths, and line ranges against .cache/corpus",
    )
    args = parser.parse_args()

    report = Report()
    feature_schema = load_schema("feature.schema.json")
    evidence_schema = load_schema("evidence.schema.json")
    library_schema = load_schema("library.schema.json")
    corpus_schema = load_schema("corpus.schema.json")
    compilers_schema = load_schema("compilers.schema.json")

    features: dict[str, dict] = {}
    feature_bodies: dict[str, str] = {}
    feature_paths: dict[str, Path] = {}
    feature_files = sorted(FEATURE_DIR.glob("*.md")) if FEATURE_DIR.exists() else []
    for path in feature_files:
        if path.name.startswith("_"):
            continue
        parsed = read_frontmatter(path, report)
        if parsed is None:
            continue
        data, body = parsed
        validate_schema(report, path, data, feature_schema)
        feature_id = data.get("id")
        if not isinstance(feature_id, str):
            continue
        if path.stem != feature_id:
            report.error(path, f"filename stem {path.stem!r} != id {feature_id!r}")
        check_standard(report, path, feature_id, data.get("standard"))
        check_date(report, path, "last_verified", data.get("last_verified"))
        if feature_id in features:
            report.error(path, f"duplicate feature id {feature_id}")
        features[feature_id] = data
        feature_bodies[feature_id] = body
        feature_paths[feature_id] = path

    if not COMPILERS_FILE.exists():
        report.error(COMPILERS_FILE, "missing compiler minimum table")
    else:
        try:
            compilers = yaml.safe_load(COMPILERS_FILE.read_text(encoding="utf-8"))
        except yaml.YAMLError as exc:
            report.error(COMPILERS_FILE, f"invalid YAML: {exc}")
            compilers = {}
        validate_schema(report, COMPILERS_FILE, compilers, compilers_schema)
        check_compilers(report, features, compilers)

    evidence: dict[str, dict] = {}
    evidence_by_feature: dict[str, list[dict]] = defaultdict(list)
    evidence_files = (
        sorted(EVIDENCE_DIR.glob("*.json")) if EVIDENCE_DIR.exists() else []
    )
    evidence_keys: dict[tuple[str, str, str, str, str], Path] = {}
    for path in evidence_files:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            report.error(path, f"invalid JSON: {exc}")
            continue
        if not validate_schema(report, path, data, evidence_schema):
            continue
        evidence_id = data["id"]
        if path.stem != evidence_id:
            report.error(path, f"filename stem {path.stem!r} != id {evidence_id!r}")
        check_date(report, path, "verified_at", data.get("verified_at"))
        generated_review = data.get("generated_review")
        if isinstance(generated_review, dict):
            check_date(
                report,
                path,
                "generated_review.approved_at",
                generated_review.get("approved_at"),
            )
        if evidence_id in evidence:
            report.error(path, f"duplicate evidence id {evidence_id}")
        evidence[evidence_id] = data
        evidence_by_feature[data["feature"]].append(data)

        key = (
            data["repo"],
            data["commit"],
            data["path"],
            data["lines"],
            data["feature"],
        )
        if key in evidence_keys:
            report.error(
                path, f"duplicate evidence location, first at {evidence_keys[key]}"
            )
        else:
            evidence_keys[key] = path

        if data["feature"] not in features:
            report.error(path, f"feature {data['feature']} does not exist")
        if len(data["excerpt"].splitlines()) > MAX_EXCERPT_LINES:
            report.error(path, f"excerpt exceeds {MAX_EXCERPT_LINES} lines")
        if data["license"] not in EXCERPT_LICENSES and data["excerpt"].strip():
            report.error(
                path,
                f"license {data['license']} allows location only, but excerpt is not empty",
            )
        check_evidence_review(report, path, data)
        check_generated_evidence(report, path, data)

    for feature_id, data in features.items():
        path = feature_paths[feature_id]
        feature_evidence = evidence_by_feature[feature_id]
        check_tier(
            report,
            path,
            data,
            feature_bodies[feature_id],
            feature_evidence,
        )
        check_evidence_mix(report, path, feature_evidence)
        check_claims(report, path, data, feature_evidence)
        check_review(report, path, data)

    libraries: dict[str, dict] = {}
    library_files = sorted(LIBRARY_DIR.glob("*.yaml")) if LIBRARY_DIR.exists() else []
    for path in library_files:
        try:
            data = yaml.safe_load(path.read_text(encoding="utf-8"))
        except yaml.YAMLError as exc:
            report.error(path, f"invalid YAML: {exc}")
            continue
        if not validate_schema(report, path, data, library_schema):
            continue
        library_id = data["id"]
        if path.stem != library_id:
            report.error(path, f"filename stem {path.stem!r} != id {library_id!r}")
        if library_id in libraries:
            report.error(path, f"duplicate library id {library_id}")
        libraries[library_id] = data
        if data["feature"] not in features:
            report.error(path, f"feature {data['feature']} does not exist")
        for evidence_id in data["evidence"]:
            if evidence_id not in evidence:
                report.error(path, f"evidence {evidence_id} does not exist")

    if not CORPUS_FILE.exists():
        report.error(CORPUS_FILE, "missing corpus manifest")
        corpus: list[dict] = []
    else:
        try:
            corpus = yaml.safe_load(CORPUS_FILE.read_text(encoding="utf-8"))
        except yaml.YAMLError as exc:
            report.error(CORPUS_FILE, f"invalid YAML: {exc}")
            corpus = []
        if not validate_schema(report, CORPUS_FILE, corpus, corpus_schema):
            corpus = corpus if isinstance(corpus, list) else []

    corpus_by_repo: dict[str, dict] = {}
    for entry in corpus:
        if not isinstance(entry, dict):
            continue
        repo = entry.get("repo")
        if not isinstance(repo, str):
            continue
        if repo in corpus_by_repo:
            report.error(CORPUS_FILE, f"duplicate corpus repo {repo}")
        corpus_by_repo[repo] = entry

    if args.check_corpus:
        for path in evidence_files:
            try:
                data = json.loads(path.read_text(encoding="utf-8"))
            except json.JSONDecodeError:
                continue
            if (
                isinstance(data, dict)
                and {"repo", "commit", "path", "lines"} <= data.keys()
            ):
                check_corpus_link(report, path, data, corpus_by_repo)

    if report.errors:
        for message in report.errors:
            print(f"ERROR {message}", file=sys.stderr)
        print(f"{len(report.errors)} error(s)", file=sys.stderr)
        return 1

    print(
        f"OK: {len(features)} features, {len(evidence)} evidence entries, "
        f"{len(libraries)} library cards, {len(corpus)} corpus repos"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
