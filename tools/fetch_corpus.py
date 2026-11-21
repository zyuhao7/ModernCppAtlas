#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = ["pyyaml>=6.0"]
# ///
"""Fetch pinned corpus checkouts or upstream heads for discovery."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / ".cache" / "corpus"
LATEST_CACHE = ROOT / ".cache" / "corpus-latest"


def run(cmd: list[str], *, cwd: Path | None = None) -> str:
    result = subprocess.run(cmd, cwd=cwd, text=True, capture_output=True)
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip()
        raise SystemExit(f"command failed: {' '.join(cmd)}: {detail}")
    return result.stdout.strip()


def head_commit(path: Path) -> str:
    return run(["git", "rev-parse", "HEAD"], cwd=path)


def require_clean(repo: str, dest: Path) -> None:
    if run(["git", "status", "--porcelain"], cwd=dest):
        raise SystemExit(
            f"{repo}: cache at {dest} has local changes. "
            "Commit or discard them before updating."
        )


def fetch_latest(repo: str, url: str, dest: Path) -> str:
    if dest.exists():
        require_clean(repo, dest)
        run(["git", "fetch", "--depth", "1", "--no-tags", "origin", "HEAD"], cwd=dest)
        run(["git", "checkout", "--detach", "FETCH_HEAD"], cwd=dest)
    else:
        run(
            [
                "git",
                "clone",
                "--depth",
                "1",
                "--single-branch",
                "--no-tags",
                url,
                str(dest),
            ]
        )
    return head_commit(dest)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--corpus", type=Path, default=ROOT / "data" / "corpus.yaml")
    parser.add_argument(
        "--latest",
        action="store_true",
        help="fetch upstream HEADs into .cache/corpus-latest",
    )
    args = parser.parse_args()

    entries = yaml.safe_load(args.corpus.read_text())
    cache = LATEST_CACHE if args.latest else CACHE
    cache.mkdir(parents=True, exist_ok=True)
    commits: dict[str, str] = {}
    failed_fetches: dict[str, str] = {}
    if args.latest:
        report = cache / "fetch-report.json"
        # 拉取中断后，扫描器不能继续使用上一轮的成功清单。
        report.unlink(missing_ok=True)

    for entry in entries:
        repo = entry["repo"]
        url = entry["url"]
        dest = cache / repo.replace("/", "__")
        invalid_checkout = dest.exists() and not (dest / ".git").exists()
        if args.latest:
            try:
                if invalid_checkout:
                    raise SystemExit(f"{dest} exists but is not a git checkout")
                commits[repo] = fetch_latest(repo, url, dest)
            except SystemExit as exc:
                failed_fetches[repo] = str(exc)
                print(f"warning: {repo}: {exc}", file=sys.stderr)
            else:
                print(f"{repo}: upstream HEAD ready at {commits[repo][:12]}")
            continue

        if invalid_checkout:
            raise SystemExit(f"{dest} exists but is not a git checkout")
        commit = entry["commit"]
        tag = entry["tag"]
        if dest.exists():
            actual = head_commit(dest)
            if actual == commit:
                print(f"{repo}: cached at {actual[:12]}")
                continue
            require_clean(repo, dest)
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

    if args.latest:
        report.write_text(
            json.dumps({"commits": commits, "failed_fetches": failed_fetches}, indent=2)
            + "\n",
            encoding="utf-8",
        )
        if not commits:
            print("error: no repositories were fetched", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
