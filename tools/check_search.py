#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = ["jieba>=0.42"]
# ///
"""Smoke-test Chinese search coverage in a built MkDocs site."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import jieba

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_QUERIES = [
    "结构化绑定",
    "std::optional",
    "std::variant",
    "std::scoped_lock",
    "std::shared_mutex",
    "std::any",
    "折叠表达式",
    "协程 co_await",
    "span 生命周期",
    "证据 固定 commit",
    "mold insert",
    "按使用场景",
]


def is_cjk(text: str) -> bool:
    return any("一" <= char <= "鿿" for char in text)


def query_tokens(query: str) -> list[str]:
    tokens: list[str] = []
    for part in query.split():
        if is_cjk(part):
            tokens.extend(
                token.strip().lower()
                for token in jieba.cut_for_search(part)
                if token.strip()
            )
        else:
            tokens.append(part.lower())
    return tokens


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--site-dir", type=Path, default=ROOT / "site")
    parser.add_argument("--query", action="append", dest="queries")
    args = parser.parse_args()

    index_path = args.site_dir / "search" / "search_index.json"
    if not index_path.exists():
        print(f"missing search index: {index_path}", file=sys.stderr)
        return 1

    payload = json.loads(index_path.read_text(encoding="utf-8"))
    docs = payload.get("docs", [])
    if not docs:
        print("search index has no documents", file=sys.stderr)
        return 1

    haystacks = [
        (
            doc.get("location", ""),
            f"{doc.get('title', '')} {doc.get('text', '')}".lower(),
        )
        for doc in docs
    ]

    failures: list[str] = []
    queries = args.queries or DEFAULT_QUERIES
    for query in queries:
        tokens = query_tokens(query)
        if not tokens:
            failures.append(f"{query}: empty query after tokenization")
            continue
        matches = [
            location
            for location, haystack in haystacks
            if all(token in haystack for token in tokens)
        ]
        status = "ok  " if matches else "FAIL"
        print(f"{status} {query!r} -> {len(matches)} doc(s) {matches[:3]}")
        if not matches:
            failures.append(f"{query}: no document contains {tokens}")

    if failures:
        for failure in failures:
            print(f"FAIL {failure}", file=sys.stderr)
        return 1
    print(f"{len(queries)} search queries hit")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
