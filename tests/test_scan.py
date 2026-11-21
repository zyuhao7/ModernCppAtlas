import contextlib
import io
import json
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import yaml

from tools import fetch_corpus
from tools.scan import scan


def git(repo: Path, *args: str) -> str:
    result = subprocess.run(
        [
            "git",
            "-c",
            "user.name=Corpus Fixture",
            "-c",
            "user.email=fixture@example.invalid",
            "-c",
            "commit.gpgsign=false",
            "-c",
            "core.hooksPath=/dev/null",
            *args,
        ],
        cwd=repo,
        check=True,
        text=True,
        capture_output=True,
    )
    return result.stdout.strip()


class CorpusScanTests(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        self.upstream = self.root / "upstream"
        self.upstream.mkdir()
        git(self.upstream, "init", "--initial-branch=main")
        (self.upstream / "src").mkdir()
        self.source = self.upstream / "src" / "original file.cpp"
        self.source.write_text("int length = std::ssize(values);\n")
        self.pin = self.commit("Pinned sources")
        git(self.upstream, "tag", "v1")
        self.pinned_cache = self.root / "pinned"
        self.latest_cache = self.root / "latest"
        self.manifest = self.root / "corpus.yaml"
        self.manifest.write_text(
            yaml.safe_dump(
                [
                    {
                        "repo": "fixture/example",
                        "url": self.upstream.as_uri(),
                        "commit": self.pin,
                        "tag": "v1",
                        "license": "MIT",
                        "paths": ["src"],
                    }
                ]
            )
        )

    def commit(self, message: str) -> str:
        git(self.upstream, "add", "--all")
        git(self.upstream, "commit", "-m", message)
        return git(self.upstream, "rev-parse", "HEAD")

    def run_fetch(self, *args: str, status: int = 0) -> None:
        with (
            patch.object(fetch_corpus, "CACHE", self.pinned_cache),
            patch.object(fetch_corpus, "LATEST_CACHE", self.latest_cache),
            patch(
                "sys.argv", ["fetch_corpus.py", "--corpus", str(self.manifest), *args]
            ),
            contextlib.redirect_stdout(io.StringIO()),
        ):
            self.assertEqual(fetch_corpus.main(), status)

    def run_scan(self, *args: str, status: int = 0) -> dict:
        output = self.root / "candidates.json"
        with (
            patch(
                "sys.argv",
                [
                    "scan.py",
                    "--corpus",
                    str(self.manifest),
                    "--output",
                    str(output),
                    *args,
                ],
            ),
            contextlib.redirect_stdout(io.StringIO()),
        ):
            self.assertEqual(scan.main(), status)
        return json.loads(output.read_text())

    def test_scan_uses_pinned_blobs_when_worktree_diverges(self) -> None:
        cache = self.pinned_cache / "fixture__example"
        self.pinned_cache.mkdir()
        self.source.rename(self.source.with_name("renamed.cpp"))
        self.commit("Rename the source on main")
        git(self.root, "clone", str(self.upstream), str(cache))
        (cache / "src" / "renamed.cpp").write_text("int length = std::ssize(dirty);\n")
        (cache / "src" / "extra.cpp").write_text("int length = std::ssize(extra);\n")

        result = self.run_scan("--cache", str(self.pinned_cache))

        self.assertEqual(len(result["candidates"]), 1)
        candidate = result["candidates"][0]
        self.assertEqual(candidate["commit"], self.pin)
        self.assertEqual(candidate["path"], "src/original file.cpp")
        self.assertEqual(candidate["excerpt"], "int length = std::ssize(values);")

    def test_latest_fetch_refreshes_default_branch_and_keeps_pin(self) -> None:
        self.run_fetch()
        self.run_fetch("--latest")
        git(self.upstream, "switch", "-c", "new-default")
        self.source.write_text("auto removed = std::erase_if(values, predicate);\n")
        latest = self.commit("Introduce a newer usage")
        self.run_fetch("--latest")

        result = self.run_scan(
            "--latest", "--cache", str(self.latest_cache), "--feature", "cpp20.erase-if"
        )
        pinned_checkout = self.pinned_cache / "fixture__example"

        self.assertEqual(git(pinned_checkout, "rev-parse", "HEAD"), self.pin)
        self.assertEqual(
            result["corpus"],
            [{"repo": "fixture/example", "commit": latest, "tag": None}],
        )
        self.assertEqual(len(result["candidates"]), 1)
        self.assertEqual(result["candidates"][0]["commit"], latest)

    def test_latest_scan_rejects_report_without_fetch_results(self) -> None:
        self.latest_cache.mkdir(parents=True)
        (self.latest_cache / "fetch-report.json").write_text("{}")
        output = self.root / "candidates.json"
        with (
            patch(
                "sys.argv",
                [
                    "scan.py",
                    "--corpus",
                    str(self.manifest),
                    "--output",
                    str(output),
                    "--latest",
                    "--cache",
                    str(self.latest_cache),
                ],
            ),
            contextlib.redirect_stdout(io.StringIO()),
            contextlib.redirect_stderr(io.StringIO()),
            self.assertRaises(SystemExit) as raised,
        ):
            scan.main()

        self.assertIn("missing fetch results", str(raised.exception))

    def test_latest_scan_skips_repos_without_checkout(self) -> None:
        self.latest_cache.mkdir(parents=True)
        (self.latest_cache / "fetch-report.json").write_text(
            json.dumps({"commits": {"fixture/example": self.pin}, "failed_fetches": {}})
        )

        with contextlib.redirect_stderr(io.StringIO()):
            result = self.run_scan(
                "--latest", "--cache", str(self.latest_cache), status=1
            )

        self.assertEqual(result["corpus"], [])
        self.assertEqual(result["candidates"], [])
        self.assertIn("fixture/example", result["failed_fetches"])

    def test_dedicated_features_keep_umbrella_matches(self) -> None:
        candidates = scan.scan_file(
            "fixture/example",
            self.pin,
            "MIT",
            Path("src/main.cpp"),
            b"std::erase_if(values, predicate);\n"
            b'if (text.ends_with(".cpp")) {}\n'
            b"int length = std::ssize(values);\n",
        )

        self.assertEqual(
            {(item["feature"], item["lines"]) for item in candidates},
            {
                ("cpp20.erase-if", "1-1"),
                ("cpp20.string-starts-ends-with", "2-2"),
                ("cpp20.string-range-utilities", "1-1"),
                ("cpp20.string-range-utilities", "2-2"),
                ("cpp20.string-range-utilities", "3-3"),
            },
        )

    def test_latest_scan_keeps_successes_and_excludes_failed_cached_repos(self) -> None:
        good = yaml.safe_load(self.manifest.read_text())[0]
        bad = {**good, "repo": "fixture/unavailable"}
        self.manifest.write_text(yaml.safe_dump([bad, good]))
        self.run_fetch("--latest")
        failed_checkout = self.latest_cache / "fixture__unavailable"
        missing_origin = (self.root / "gone").as_uri()
        git(failed_checkout, "remote", "set-url", "origin", missing_origin)

        with contextlib.redirect_stderr(io.StringIO()):
            self.run_fetch("--latest")
            result = self.run_scan("--latest", "--cache", str(self.latest_cache))
            good_checkout = self.latest_cache / "fixture__example"
            git(good_checkout, "remote", "set-url", "origin", missing_origin)
            self.run_fetch("--latest", status=1)
            failed = self.run_scan(
                "--latest", "--cache", str(self.latest_cache), status=1
            )

        self.assertEqual([entry["repo"] for entry in result["corpus"]], [good["repo"]])
        self.assertEqual(
            [entry["repo"] for entry in result["candidates"]], [good["repo"]]
        )
        self.assertIn(bad["repo"], result["failed_fetches"])
        self.assertEqual(failed["corpus"], [])
        self.assertEqual(failed["candidates"], [])
        self.assertEqual(set(failed["failed_fetches"]), {bad["repo"], good["repo"]})
