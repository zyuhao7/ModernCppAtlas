#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = ["pyyaml>=6.0"]
# ///
"""Compile marked C++ fences and require a checked basic-usage snippet."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]
FEATURE_DIR = ROOT / "data" / "features"
FENCE = re.compile(r"^```([^\n`]*)\n(.*?)^```[ \t]*$", re.MULTILINE | re.DOTALL)
BASIC = "## 基础用法"
MARKER = "<!-- compile -->"


@dataclass
class Fence:
    language: str
    code: str
    line: int
    marked: bool


def split_frontmatter(text: str) -> tuple[dict, str, int]:
    if not text.startswith("---\n"):
        raise ValueError("missing frontmatter")
    end = text.find("\n---\n", 4)
    if end == -1:
        raise ValueError("unterminated frontmatter")
    data = yaml.safe_load(text[4:end]) or {}
    return data, text[end + 5 :], text[: end + 5].count("\n")


def fences(body: str, offset: int) -> list[Fence]:
    result: list[Fence] = []
    for match in FENCE.finditer(body):
        prefix = body[: match.start()].rstrip("\n").splitlines()
        info = match.group(1).strip()
        result.append(
            Fence(
                language=info.split()[0] if info else "",
                code=match.group(2),
                line=offset + body[: match.start()].count("\n") + 1,
                marked=bool(prefix) and prefix[-1].strip() == MARKER,
            )
        )
    return result


def basic_fence(body: str, items: list[Fence], offset: int) -> Fence | None:
    start = body.find(BASIC)
    if start == -1:
        return None
    heading_line = offset + body[:start].count("\n") + 1
    end = body.find("\n## ", start + len(BASIC))
    end_line = offset + body[:end].count("\n") + 1 if end != -1 else float("inf")
    return next(
        (
            item
            for item in items
            if item.language == "cpp" and heading_line < item.line < end_line
        ),
        None,
    )


def compile_snippet(
    path: Path, item: Fence, standard: str, compiler: str, failures: list[str]
) -> bool:
    command = [
        compiler,
        "-std=c++" + standard.removeprefix("C++"),
        "-fsyntax-only",
        "-x",
        "c++",
        "-",
    ]
    result = subprocess.run(command, input=item.code, text=True, capture_output=True)
    if result.returncode == 0:
        return True
    detail = result.stderr.strip().splitlines()
    message = detail[0] if detail else "compiler rejected snippet"
    failures.append(f"{path}:{item.line}: marked snippet does not compile: {message}")
    return False


def check_file(path: Path, compiler: str, failures: list[str]) -> tuple[int, int]:
    text = path.read_text(encoding="utf-8")
    try:
        data, body, offset = split_frontmatter(text)
    except ValueError as exc:
        failures.append(f"{path}: {exc}")
        return 0, 0
    items = fences(body, offset)
    cpp = [item for item in items if item.language == "cpp"]
    compiled = sum(
        compile_snippet(
            path, item, str(data.get("standard", "C++17")), compiler, failures
        )
        for item in cpp
        if item.marked
    )

    if data.get("tier") in {"A", "B"}:
        first = basic_fence(body, items, offset)
        if first is None:
            failures.append(f"{path}: missing C++ fence in {BASIC}")
        elif not first.marked:
            failures.append(
                f"{path}:{first.line}: first {BASIC} fence needs a {MARKER} marker"
            )
    return len(cpp), compiled


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()

    failures: list[str] = []
    total = compiled = 0
    for path in sorted(FEATURE_DIR.glob("*.md")):
        if path.name.startswith("_"):
            continue
        counts = check_file(path, args.compiler, failures)
        total += counts[0]
        compiled += counts[1]

    if failures:
        for failure in failures:
            print(f"FAIL {failure}", file=sys.stderr)
        return 1
    print(f"{total} C++ fences, {compiled} marked fences compiled")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
