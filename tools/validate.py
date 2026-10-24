#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = ["jsonschema>=4.23", "pyyaml>=6.0"]
# ///
"""Validate feature, evidence, library, and corpus data."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

import yaml
from jsonschema import Draft202012Validator

ROOT = Path(__file__).resolve().parents[1]
SCHEMA_DIR = ROOT / "schema"
FEATURE_DIR = ROOT / "data" / "features"
EVIDENCE_DIR = ROOT / "data" / "evidence"
LIBRARY_DIR = ROOT / "data" / "libraries"
CORPUS_FILE = ROOT / "data" / "corpus.yaml"
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


class Report:
    def __init__(self) -> None:
        self.errors: list[str] = []

    def error(self, path: Path, message: str) -> None:
        self.errors.append(f"{path.relative_to(ROOT)}: {message}")


def load_schema(name: str) -> dict:
    return json.loads((SCHEMA_DIR / name).read_text())


def validate_schema(report: Report, path: Path, data: object, schema: dict) -> bool:
    validator = Draft202012Validator(schema)
    errors = sorted(validator.iter_errors(data), key=lambda error: list(error.path))
    for error in errors:
        location = ".".join(str(part) for part in error.path) or "<root>"
        report.error(path, f"{location}: {error.message}")
    return not errors


def read_frontmatter(path: Path, report: Report) -> dict | None:
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
    return data


def git(repo_dir: Path, *args: str) -> subprocess.CompletedProcess:
    return subprocess.run(["git", *args], cwd=repo_dir, text=True, capture_output=True)


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
    if evidence["commit"] != entry["commit"]:
        report.error(
            path,
            f"commit {evidence['commit']} differs from pinned corpus commit {entry['commit']}",
        )
        return
    repo_dir = CORPUS_CACHE / repo.replace("/", "__")
    if not (repo_dir / ".git").exists():
        report.error(path, f"corpus cache missing: {repo_dir}")
        return
    head = git(repo_dir, "rev-parse", "HEAD")
    if head.returncode != 0 or head.stdout.strip() != evidence["commit"]:
        report.error(path, f"corpus checkout is not at {evidence['commit']}")
        return
    object_spec = f"{evidence['commit']}:{evidence['path']}"
    exists = git(repo_dir, "cat-file", "-e", object_spec)
    if exists.returncode != 0:
        report.error(path, f"path not found at pinned commit: {evidence['path']}")
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
    line_count = len(shown.stdout.splitlines())
    if end > line_count:
        report.error(path, f"line range {evidence['lines']} exceeds {line_count} lines")


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

    features: dict[str, dict] = {}
    feature_files = sorted(FEATURE_DIR.glob("*.md")) if FEATURE_DIR.exists() else []
    for path in feature_files:
        if path.name.startswith("_"):
            continue
        data = read_frontmatter(path, report)
        if data is None:
            continue
        validate_schema(report, path, data, feature_schema)
        feature_id = data.get("id")
        if not isinstance(feature_id, str):
            continue
        if path.stem != feature_id:
            report.error(path, f"filename stem {path.stem!r} != id {feature_id!r}")
        if feature_id in features:
            report.error(path, f"duplicate feature id {feature_id}")
        features[feature_id] = data

    evidence: dict[str, dict] = {}
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
        if evidence_id in evidence:
            report.error(path, f"duplicate evidence id {evidence_id}")
        evidence[evidence_id] = data

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
        if data["level"] == "manual" and (
            len(data["verified_by"]) < 1 or data["verified_at"] is None
        ):
            report.error(path, "manual evidence needs a reviewer and verified_at")
        if data["level"] == "verified" and (
            len(data["verified_by"]) < 2 or data["verified_at"] is None
        ):
            report.error(path, "verified evidence needs two reviewers and verified_at")
        if data["level"] == "candidate" and data["verified_by"]:
            report.error(path, "candidate evidence must not list reviewers")

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
