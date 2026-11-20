#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = ["pyyaml>=6.0"]
# ///
"""Check that published evidence still resolves upstream.

Pinned permalinks do not drift, so this tool does not re-review excerpts.
It only detects structural failures: repository gone, commit unreachable,
path missing at that commit, or a license change. Network failures are
reported as `unknown` and never demote evidence.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import re
import sys
import urllib.error
import urllib.parse
import urllib.request
from collections import Counter
from datetime import date, datetime, timezone
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE_DIR = ROOT / "data" / "evidence"
DEFAULT_REPORT = ROOT / ".cache" / "freshness.json"
DEFAULT_TIMEOUT = 20.0
API = "https://api.github.com"
PUBLISHED_LEVELS = {"manual", "verified"}
STRUCTURAL = {"repo_missing", "commit_missing", "path_missing", "license_changed"}


class NetworkError(Exception):
    """Transient failure: the evidence cannot be judged this run."""


def fetch_json(url: str, token: str | None, timeout: float = DEFAULT_TIMEOUT):
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "ModernCppAtlas-freshness",
        "X-GitHub-Api-Version": "2022-11-28",
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    request = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            return response.status, json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        if exc.code in {403, 429} and "rate limit" in body.lower():
            raise NetworkError(f"rate limited: HTTP {exc.code}") from exc
        if exc.code >= 500:
            raise NetworkError(f"HTTP {exc.code}") from exc
        try:
            payload = json.loads(body)
        except json.JSONDecodeError:
            payload = {"message": body}
        return exc.code, payload
    except (urllib.error.URLError, TimeoutError, OSError) as exc:
        raise NetworkError(str(exc)) from exc


class Checker:
    def __init__(self, token: str | None, timeout: float = DEFAULT_TIMEOUT) -> None:
        self.token = token
        self.timeout = timeout
        self.repositories: dict[str, tuple[str, str]] = {}
        self.licenses: dict[str, tuple[str, str | None]] = {}

    def fetch(self, url: str):
        return fetch_json(url, self.token, self.timeout)

    def check_repository(self, repo: str) -> tuple[str, str]:
        if repo not in self.repositories:
            status, _payload = self.fetch(f"{API}/repos/{repo}")
            if status == 200:
                self.repositories[repo] = ("ok", "")
            elif status == 404:
                self.repositories[repo] = (
                    "repo_missing",
                    "repository not found (deleted or private)",
                )
            else:
                raise NetworkError(f"repository lookup failed: HTTP {status}")
        return self.repositories[repo]

    def check_license(self, repo: str) -> tuple[str, str | None]:
        if repo not in self.licenses:
            status, payload = self.fetch(f"{API}/repos/{repo}/license")
            if status == 200 and isinstance(payload, dict):
                license_info = payload.get("license") or {}
                spdx = license_info.get("spdx_id")
                if isinstance(spdx, str) and spdx not in {"NOASSERTION", "NONE"}:
                    self.licenses[repo] = ("ok", spdx)
                else:
                    self.licenses[repo] = ("unknown", "no detectable SPDX license")
            elif status == 404:
                self.licenses[repo] = ("unknown", "no license file detected")
            else:
                raise NetworkError(f"license lookup failed: HTTP {status}")
        return self.licenses[repo]

    def check(self, evidence: dict) -> tuple[str, str]:
        try:
            return self.resolve(evidence)
        except NetworkError as exc:
            return "unknown", str(exc)

    def resolve(self, evidence: dict) -> tuple[str, str]:
        repo = evidence["repo"]
        commit = evidence["commit"]
        path = evidence["path"]

        status, detail = self.check_repository(repo)
        if status != "ok":
            return status, detail

        status, _payload = self.fetch(f"{API}/repos/{repo}/commits/{commit}")
        if status in {404, 422}:
            return "commit_missing", f"commit {commit} not found (history rewritten?)"
        if status != 200:
            raise NetworkError(f"commit lookup failed: HTTP {status}")

        quoted = urllib.parse.quote(path, safe="/")
        url = f"{API}/repos/{repo}/contents/{quoted}?ref={commit}"
        status, payload = self.fetch(url)
        message = str(payload.get("message", "")) if isinstance(payload, dict) else ""
        if status == 404:
            return "path_missing", f"{path} not found at {commit[:12]}"
        if status == 403 and "large" in message.lower():
            pass  # oversized blob: the path exists, contents API refuses to inline it
        elif status != 200:
            raise NetworkError(f"contents lookup failed: HTTP {status}")

        status, spdx = self.check_license(repo)
        if status != "ok" or spdx is None:
            return "unknown", f"license: {spdx}"
        if spdx != evidence["license"]:
            return "license_changed", f"expected {evidence['license']}, found {spdx}"
        return "ok", ""


INLINE_ARRAYS = re.compile(r'\[\n\s+((?:"[^"\n]*"(?:,\s*\n\s+)?)+)\n\s+\]')


def serialize_evidence(data: dict, original: str) -> str:
    """Round-trip a file without reformatting unrelated lines."""
    plain = json.dumps(data, indent=2, ensure_ascii=False) + "\n"
    if re.search(r'"verified_by": \[\s*\n', original):
        return plain
    return INLINE_ARRAYS.sub(
        lambda match: "[" + re.sub(r",\s*\n\s+", ", ", match.group(1)) + "]",
        plain,
    )


def apply_stale(path: Path, record: dict, today: str) -> str | None:
    original = path.read_text(encoding="utf-8")
    data = json.loads(original)
    if data.get("level") == "stale":
        return None
    data["level"] = "stale"
    data["stale_reason"] = f"{record['status']}: {record['detail']}"
    data["stale_since"] = today
    path.write_text(serialize_evidence(data, original), encoding="utf-8")
    return data["feature"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument(
        "--apply-stale",
        action="store_true",
        help="demote evidence with a structural failure to level: stale",
    )
    parser.add_argument("--repo", help="only check one repository")
    parser.add_argument(
        "--timeout", type=float, default=DEFAULT_TIMEOUT, help="HTTP timeout in seconds"
    )
    args = parser.parse_args()
    if not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("--timeout must be a finite positive number")

    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    if not token:
        print(
            "warning: no GITHUB_TOKEN/GH_TOKEN; unauthenticated API allows 60 "
            "requests per hour",
            file=sys.stderr,
        )
    checker = Checker(token, args.timeout)

    records: list[dict] = []
    for path in sorted(EVIDENCE_DIR.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        if data.get("level") not in PUBLISHED_LEVELS:
            continue
        if args.repo and data["repo"] != args.repo:
            continue
        record = {
            "id": data["id"],
            "feature": data["feature"],
            "repo": data["repo"],
            "commit": data["commit"],
            "path": data["path"],
            "file": path.relative_to(ROOT).as_posix(),
        }
        status, detail = checker.check(data)
        record["status"] = status
        record["detail"] = detail
        records.append(record)

    counts = Counter(record["status"] for record in records)
    payload = {
        "generated_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "counts": dict(sorted(counts.items())),
        "problems": [record for record in records if record["status"] != "ok"],
        "records": records,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(
        json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )

    print(f"checked {len(records)} published evidence entries")
    for status, count in sorted(counts.items()):
        print(f"  {status}: {count}")

    failures = [record for record in records if record["status"] in STRUCTURAL]
    for record in failures:
        print(f"  {record['id']}: {record['status']} ({record['detail']})")
    unknowns = [record for record in records if record["status"] == "unknown"]
    for record in unknowns:
        print(f"  warning: {record['id']}: unknown ({record['detail']})")

    if args.apply_stale and failures:
        failed_ids = {record["id"] for record in failures}
        affected_cards = []
        for path in sorted((ROOT / "data" / "libraries").glob("*.yaml")):
            card = yaml.safe_load(path.read_text(encoding="utf-8"))
            if failed_ids.intersection(card["evidence"]):
                affected_cards.append(path.relative_to(ROOT).as_posix())
        today = date.today().isoformat()
        affected: set[str] = set()
        for record in failures:
            feature = apply_stale(ROOT / record["file"], record, today)
            if feature:
                affected.add(feature)
        if affected:
            print(
                "stale evidence removed from these features; update their claims "
                "ledgers before validating: " + ", ".join(sorted(affected))
            )
        if affected_cards:
            print(
                "update evidence references in these library cards before validating: "
                + ", ".join(affected_cards)
            )
        return 0

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
