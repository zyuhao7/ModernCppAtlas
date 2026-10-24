#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = ["pyyaml>=6.0", "tree-sitter>=0.25", "tree-sitter-cpp>=0.23"]
# ///
"""Emit candidate evidence locations from the pinned corpus.

This is a discovery tool, not a verifier. Every candidate still needs human
review before it can enter data/evidence.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

import tree_sitter_cpp
import yaml
from tree_sitter import Language, Parser

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CACHE = ROOT / ".cache" / "corpus"
DEFAULT_OUTPUT = ROOT / ".cache" / "candidates.json"

SOURCE_SUFFIXES = {
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".inl",
    ".ipp",
    ".tpp",
    ".tcc",
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
}
EXCLUDED_DIRS = {
    ".git",
    ".github",
    "build",
    "builds",
    "cmake-build-debug",
    "cmake-build-release",
    "third_party",
    "third-party",
    "3rdparty",
    "vendor",
    "vendored",
    "external",
    "extern",
    "deps",
    "_deps",
    "node_modules",
    "single_include",
    "docs",
    "doc",
    "bundled",
    "bundled_fmtlib",
}
MAX_FILE_BYTES = 2 * 1024 * 1024
MAX_EXCERPT_LINES = 15
MAX_EXCERPT_CHARS = 1200
MAX_SNIPPET_PADDING = 2


def node_text(node) -> str:
    return node.text.decode("utf-8", errors="replace")


def is_excluded(relative: Path) -> bool:
    return any(part in EXCLUDED_DIRS for part in relative.parts)


def is_test_path(relative: Path) -> bool:
    parts = {part.lower() for part in relative.parts}
    if parts & {"test", "tests", "testing", "unittest", "unittests"}:
        return True
    stem = relative.stem.lower()
    return stem.endswith("_test") or stem.startswith("test_") or stem.endswith("-test")


def iter_nodes(root):
    stack = [root]
    while stack:
        node = stack.pop()
        yield node
        stack.extend(node.children)


NODE_MATCHERS = {
    "cpp17.structured-bindings": (
        lambda node, text: node.type == "structured_binding_declarator"
    ),
    "cpp17.if-constexpr": (
        lambda node, text: (
            node.type == "if_statement"
            and any(child.type == "constexpr" for child in node.children)
        )
    ),
    "cpp17.constexpr-lambda": (
        lambda node, text: (
            node.type == "lambda_expression"
            and any(
                child.type == "abstract_function_declarator"
                and "constexpr" in node_text(child)
                for child in node.children
            )
        )
    ),
    "cpp17.fold-expressions": (lambda node, text: node.type == "fold_expression"),
    "cpp20.concepts": (
        lambda node, text: (
            node.type
            in {"concept_definition", "requires_clause", "requires_expression"}
        )
    ),
    "cpp20.coroutines": (
        lambda node, text: (
            node.type
            in {"co_await_expression", "co_yield_expression", "co_return_statement"}
        )
    ),
    "cpp20.modules": (
        lambda node, text: node.type in {"module_declaration", "import_declaration"}
    ),
    "cpp20.template-lambda": (
        lambda node, text: (
            node.type == "lambda_expression"
            and any(child.type == "template_parameter_list" for child in node.children)
        )
    ),
    "cpp20.spaceship": (
        lambda node, text: node.type == "binary_expression" and "<=>" in text
    ),
}


REGEX_QUERIES = {
    "cpp17.optional": r"\bstd::optional\b",
    "cpp17.variant": r"\bstd::variant\b",
    "cpp17.string-view": r"\bstd::string_view\b",
    "cpp17.filesystem": r"\bstd::filesystem\b",
    "cpp17.any": r"\bstd::any\b",
    "cpp17.scoped-lock": r"\bstd::scoped_lock\b",
    "cpp17.shared-mutex": r"\bstd::shared_mutex\b",
    "cpp17.parallel-algorithms": r"\bstd::execution::(?:par|par_unseq)\b",
    "cpp17.byte": r"\bstd::byte\b",
    "cpp17.pmr": r"\bstd::pmr\b",
    "cpp20.ranges": r"\bstd::(?:ranges|views)::",
    "cpp20.span": r"\bstd::span\b",
    "cpp20.format": r"\bstd::format\b",
    "cpp20.jthread-stop-token": r"\bstd::(?:jthread|stop_token|stop_source)\b",
    "cpp20.source-location": r"\bstd::source_location\b",
    "cpp20.bit-cast": r"\bstd::bit_cast\b",
    "cpp20.atomic-ref": r"\bstd::atomic_ref\b",
    "cpp20.latch-barrier-semaphore": (
        r"\bstd::(?:latch|barrier|counting_semaphore|binary_semaphore)\b"
    ),
    "cpp20.remove-cvref-type-identity": (r"\bstd::(?:remove_cvref|type_identity)\b"),
    "cpp20.string-range-utilities": (
        r"\bstd::(?:ssize|erase_if)\b|\b(?:starts_with|ends_with)\s*\("
    ),
}


def make_excerpt(lines: list[str], start: int, end: int) -> str:
    low = max(1, start - MAX_SNIPPET_PADDING)
    high = min(len(lines), end + MAX_SNIPPET_PADDING)
    chunk = lines[low - 1 : high]
    if len(chunk) > MAX_EXCERPT_LINES:
        chunk = chunk[:MAX_EXCERPT_LINES]
    text = "\n".join(chunk)
    return text[:MAX_EXCERPT_CHARS]


def candidate_id(repo: str, commit: str, path: str, feature: str, lines: str) -> str:
    digest = hashlib.sha1(
        f"{repo}:{commit}:{path}:{feature}:{lines}".encode("utf-8")
    ).hexdigest()
    return f"cand-{digest[:16]}"


def scan_file(
    repo: str,
    commit: str,
    license_name: str,
    relative: Path,
    source: bytes,
) -> list[dict]:
    text = source.decode("utf-8", errors="replace")
    lines = text.splitlines()
    if not lines:
        return []

    parser = Parser(Language(tree_sitter_cpp.language()))
    tree = parser.parse(source)
    found: list[dict] = []
    seen: set[tuple[str, int, int]] = set()

    for node in iter_nodes(tree.root_node):
        for feature, matcher in NODE_MATCHERS.items():
            snippet_text = node_text(node)
            if not matcher(node, snippet_text):
                continue
            start = node.start_point[0] + 1
            end = node.end_point[0] + 1
            key = (feature, start, end)
            if key in seen:
                continue
            seen.add(key)
            found.append(
                {
                    "repo": repo,
                    "commit": commit,
                    "path": relative.as_posix(),
                    "lines": f"{start}-{end}",
                    "feature": feature,
                    "matched_by": "tree-sitter",
                    "is_test": is_test_path(relative),
                    "license": license_name,
                    "excerpt": make_excerpt(lines, start, end),
                }
            )

    for feature, pattern in REGEX_QUERIES.items():
        regex = re.compile(pattern)
        for index, line in enumerate(lines, start=1):
            stripped = line.strip()
            if not stripped or stripped.startswith(("#include", "//", "/*", "*")):
                continue
            if not regex.search(line):
                continue
            key = (feature, index, index)
            if key in seen:
                continue
            seen.add(key)
            found.append(
                {
                    "repo": repo,
                    "commit": commit,
                    "path": relative.as_posix(),
                    "lines": f"{index}-{index}",
                    "feature": feature,
                    "matched_by": "regex",
                    "is_test": is_test_path(relative),
                    "license": license_name,
                    "excerpt": make_excerpt(lines, index, index),
                }
            )

    for item in found:
        item["candidate_id"] = candidate_id(
            item["repo"], item["commit"], item["path"], item["feature"], item["lines"]
        )
    return found


def source_files(repo_dir: Path, configured_paths: list[str]) -> list[Path]:
    roots = (
        [repo_dir / path for path in configured_paths]
        if configured_paths
        else [repo_dir]
    )
    files: list[Path] = []
    for root in roots:
        if not root.exists():
            print(f"warning: missing corpus path {root}", file=sys.stderr)
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
                continue
            relative = path.relative_to(repo_dir)
            if is_excluded(relative):
                continue
            if path.stat().st_size > MAX_FILE_BYTES:
                continue
            files.append(path)
    return sorted(files)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--corpus", type=Path, default=ROOT / "data" / "corpus.yaml")
    parser.add_argument("--cache", type=Path, default=DEFAULT_CACHE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--repo", help="only scan one corpus repo")
    parser.add_argument("--feature", help="only keep one feature id")
    args = parser.parse_args()

    entries = yaml.safe_load(args.corpus.read_text())
    candidates: list[dict] = []
    scanned_files = Counter()
    scanned_repos = Counter()

    for entry in entries:
        repo = entry["repo"]
        if args.repo and repo != args.repo:
            continue
        repo_dir = args.cache / repo.replace("/", "__")
        if not (repo_dir / ".git").exists():
            raise SystemExit(f"{repo_dir} missing; run tools/fetch_corpus.py first")

        for path in source_files(repo_dir, entry["paths"]):
            relative = path.relative_to(repo_dir)
            try:
                source = path.read_bytes()
            except OSError as exc:
                print(f"warning: cannot read {path}: {exc}", file=sys.stderr)
                continue
            found = scan_file(repo, entry["commit"], entry["license"], relative, source)
            if args.feature:
                found = [item for item in found if item["feature"] == args.feature]
            scanned_files[repo] += 1
            candidates.extend(found)

        scanned_repos[repo] = scanned_files[repo]

    candidates.sort(
        key=lambda item: (item["feature"], item["repo"], item["path"], item["lines"])
    )
    payload = {
        "generated_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "corpus": [
            {"repo": entry["repo"], "commit": entry["commit"], "tag": entry["tag"]}
            for entry in entries
            if entry["repo"] in scanned_repos
        ],
        "candidates": candidates,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n")

    print(f"scanned {sum(scanned_files.values())} files in {len(scanned_repos)} repos")
    for repo, count in sorted(scanned_files.items()):
        print(f"  {repo}: {count} files")
    feature_counts = Counter(item["feature"] for item in candidates)
    for feature, count in sorted(feature_counts.items()):
        print(f"  {feature}: {count} candidates")
    print(f"total: {len(candidates)} candidates -> {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
