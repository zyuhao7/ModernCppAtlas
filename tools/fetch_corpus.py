#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = ["pyyaml>=6.0"]
# ///
"""Clone pinned corpus repositories into .cache/corpus."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / ".cache" / "corpus"


def run(cmd: list[str], *, cwd: Path | None = None) -> str:
    result = subprocess.run(cmd, cwd=cwd, text=True, capture_output=True)
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        raise SystemExit(f"command failed: {' '.join(cmd)}")
    return result.stdout.strip()


def head_commit(path: Path) -> str:
    return run(["git", "rev-parse", "HEAD"], cwd=path)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--corpus", type=Path, default=ROOT / "data" / "corpus.yaml")
    args = parser.parse_args()

    entries = yaml.safe_load(args.corpus.read_text())
    CACHE.mkdir(parents=True, exist_ok=True)

    for entry in entries:
        repo = entry["repo"]
        commit = entry["commit"]
        tag = entry["tag"]
        url = entry["url"]
        dest = CACHE / repo.replace("/", "__")

        if dest.exists():
            if not (dest / ".git").exists():
                raise SystemExit(f"{dest} exists but is not a git checkout")
            actual = head_commit(dest)
            if actual == commit:
                print(f"{repo}: cached at {actual[:12]}")
                continue
            dirty = run(["git", "status", "--porcelain"], cwd=dest)
            if dirty:
                raise SystemExit(
                    f"{repo}: cache at {dest} has local changes; "
                    "commit or discard them before updating"
                )
            print(f"{repo}: updating {actual[:12]} -> {commit[:12]}")
            run(["git", "fetch", "--depth", "1", "origin", commit], cwd=dest)
            run(["git", "checkout", "--detach", commit], cwd=dest)
            actual = head_commit(dest)
            if actual != commit:
                raise SystemExit(
                    f"{repo}: fetched {actual}, expected {commit}; "
                    "update data/corpus.yaml or fetch the commit directly"
                )
            print(f"{repo}: ready at {actual[:12]}")
            continue

        print(f"{repo}: cloning {tag} ({commit[:12]})")
        run(
            [
                "git",
                "clone",
                "--depth",
                "1",
                "--single-branch",
                "--branch",
                tag,
                url,
                str(dest),
            ]
        )
        actual = head_commit(dest)
        if actual != commit:
            raise SystemExit(
                f"{repo}: tag {tag} resolved to {actual}, expected {commit}; "
                "update data/corpus.yaml or fetch the commit directly"
            )
        print(f"{repo}: ready at {actual[:12]}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
