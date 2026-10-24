#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = []
# ///
"""Compile every example under examples/cppNN with the matching standard."""

from __future__ import annotations

import argparse
import difflib
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ROOT / "examples"
EXPECTED = EXAMPLES / "expected"
STANDARDS = {
    "cpp11": "c++11",
    "cpp14": "c++14",
    "cpp17": "c++17",
    "cpp20": "c++20",
    "cpp23": "c++23",
    "cpp26": "c++26",
}


def standard_for(path: Path) -> str:
    for part in path.relative_to(EXAMPLES).parts:
        if part in STANDARDS:
            return STANDARDS[part]
    raise ValueError(f"{path}: expected an examples/cppNN/ directory")


def expected_path(path: Path) -> Path:
    return EXPECTED / path.relative_to(EXAMPLES).with_suffix(".out")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", default="g++")
    parser.add_argument(
        "--run", action="store_true", help="run each binary after linking"
    )
    parser.add_argument(
        "--update-expected",
        action="store_true",
        help="run and rewrite examples/expected/cppNN/*.out snapshots",
    )
    args = parser.parse_args()
    if args.update_expected:
        args.run = True

    files = sorted(EXAMPLES.rglob("*.cpp")) if EXAMPLES.exists() else []
    if not files:
        print("no examples found")
        return 0

    failures: list[str] = []
    with tempfile.TemporaryDirectory(prefix="atlas-examples-") as tmp:
        for path in files:
            standard = standard_for(path)
            binary = Path(tmp) / path.relative_to(EXAMPLES).with_suffix("")
            binary.parent.mkdir(parents=True, exist_ok=True)
            command = [
                args.compiler,
                f"-std={standard}",
                "-Wall",
                "-Wextra",
                "-pedantic",
                str(path),
                "-o",
                str(binary),
            ]
            result = subprocess.run(command, text=True, capture_output=True)
            label = f"{path.relative_to(ROOT)} [{standard}]"
            if result.returncode != 0:
                failures.append(label)
                print(f"FAIL {label}", file=sys.stderr)
                print(result.stderr, file=sys.stderr)
                continue
            print(f"ok   {label}")
            if not args.run:
                continue
            ran = subprocess.run([str(binary)], text=True, capture_output=True)
            if ran.returncode != 0:
                failures.append(f"{label} (run)")
                print(f"FAIL {label} (run)", file=sys.stderr)
                print(ran.stderr, file=sys.stderr)
                continue
            snapshot = expected_path(path)
            if args.update_expected:
                snapshot.parent.mkdir(parents=True, exist_ok=True)
                snapshot.write_text(ran.stdout, encoding="utf-8")
                print(f"snap {snapshot.relative_to(ROOT)}")
                continue
            if not snapshot.is_file():
                failures.append(f"{label} (missing snapshot)")
                print(
                    f"FAIL {label} (missing {snapshot.relative_to(ROOT)})",
                    file=sys.stderr,
                )
                continue
            wanted = snapshot.read_text(encoding="utf-8")
            if ran.stdout != wanted:
                failures.append(f"{label} (output)")
                print(f"FAIL {label} (output differs from snapshot)", file=sys.stderr)
                diff = difflib.unified_diff(
                    wanted.splitlines(keepends=True),
                    ran.stdout.splitlines(keepends=True),
                    fromfile=str(snapshot.relative_to(ROOT)),
                    tofile="actual",
                )
                print("".join(diff), file=sys.stderr)

    if failures:
        print(f"{len(failures)} example(s) failed", file=sys.stderr)
        return 1
    print(f"{len(files)} example(s) compiled")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
