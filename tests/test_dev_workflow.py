import io
import json
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest.mock import patch

from tools import check_evidence_freshness as freshness
from tools import validate

ROOT = Path(__file__).resolve().parents[1]


class FreshnessTimeoutTests(unittest.TestCase):
    def test_cli_timeout_applies_to_all_http_requests(self) -> None:
        commit = "a" * 40
        record = {
            "id": "ev-fixture",
            "feature": "cpp17.example",
            "repo": "owner/repo",
            "commit": commit,
            "path": "code.cpp",
            "level": "manual",
            "license": "MIT",
        }
        repo_url = f"{freshness.API}/repos/{record['repo']}"
        responses = {
            repo_url: {},
            f"{repo_url}/commits/{commit}": {},
            f"{repo_url}/contents/code.cpp?ref={commit}": {},
            f"{repo_url}/license": {"license": {"spdx_id": "MIT"}},
        }

        def respond(request, *, timeout):
            response = io.BytesIO(json.dumps(responses[request.full_url]).encode())
            response.status = 200
            return response

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            report = root / "report.json"
            (root / "evidence.json").write_text(json.dumps(record), encoding="utf-8")
            args = ["freshness", "--timeout", "0.75", "--report", str(report)]
            with (
                patch.object(freshness, "ROOT", root),
                patch.object(freshness, "EVIDENCE_DIR", root),
                patch.object(sys, "argv", args),
                patch.dict("os.environ", {"GITHUB_TOKEN": "test"}),
                patch.object(
                    freshness.urllib.request, "urlopen", side_effect=respond
                ) as urlopen,
                redirect_stdout(io.StringIO()),
            ):
                self.assertEqual(freshness.main(), 0)
            self.assertEqual(json.loads(report.read_text())["counts"], {"ok": 1})
            self.assertEqual(
                [
                    (call.args[0].full_url, call.kwargs["timeout"])
                    for call in urlopen.call_args_list
                ],
                [(url, 0.75) for url in responses],
            )

    def test_stale_evidence_warns_about_and_invalidates_library_card(self) -> None:
        card_name = "lib-vroom-semaphore.yaml"
        evidence_id = "ev-vroom-semaphore-search-001"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            libraries = root / "data/libraries"
            libraries.mkdir(parents=True)
            shutil.copyfile(validate.LIBRARY_DIR / card_name, libraries / card_name)
            evidence = root / "evidence"
            evidence.mkdir()
            for path in validate.EVIDENCE_DIR.glob("*.json"):
                target = evidence / path.name
                if path.stem == evidence_id:
                    shutil.copyfile(path, target)
                else:
                    target.symlink_to(path)

            def check(_checker, record):
                if record["id"] == evidence_id:
                    return "path_missing", "fixture path is gone"
                return "ok", ""

            with (
                patch.object(freshness, "ROOT", root),
                patch.object(freshness, "EVIDENCE_DIR", evidence),
                patch.object(freshness.Checker, "check", check),
                patch.dict("os.environ", {"GITHUB_TOKEN": "test"}),
                patch(
                    "sys.argv",
                    [
                        "freshness",
                        "--apply-stale",
                        "--report",
                        str(root / "report.json"),
                    ],
                ),
                redirect_stdout(io.StringIO()) as output,
            ):
                self.assertEqual(freshness.main(), 0)
            self.assertIn(f"data/libraries/{card_name}", output.getvalue())

            with (
                patch.object(validate, "EVIDENCE_DIR", evidence),
                patch.object(validate, "LIBRARY_DIR", libraries),
                patch("sys.argv", ["validate.py"]),
                redirect_stderr(io.StringIO()) as errors,
            ):
                self.assertEqual(validate.main(), 1)
            self.assertIn(
                f"{card_name}: evidence {evidence_id} is not published",
                errors.getvalue(),
            )


@unittest.skipUnless(shutil.which("make"), "requires make")
class MakefileTests(unittest.TestCase):
    def test_explicit_example_tracks_compiler_and_flags(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            shutil.copyfile(ROOT / "Makefile", root / "Makefile")
            source = root / "examples" / "cpp11" / "example.cpp"
            source.parent.mkdir(parents=True)
            source.touch()
            compiler = root / "compiler.py"
            compiler.write_text(
                "from pathlib import Path\n"
                "import sys\n"
                "compiler, flag, *args = sys.argv[1:]\n"
                "output = Path(args[args.index('-o') + 1])\n"
                "output.write_text(f'{compiler} {flag}\\n')\n"
                "sys.exit(flag == '-DFAIL')\n",
                encoding="utf-8",
            )
            target = Path("build/examples/cpp11/example")
            executable = root / target

            def build(name, flag):
                command = (
                    f"{shlex.quote(sys.executable)} {shlex.quote(str(compiler))} {name}"
                )
                return subprocess.run(
                    ["make", str(target), f"CXX={command}", f"CXXFLAGS={flag}"],
                    cwd=root,
                    capture_output=True,
                    text=True,
                )

            for name, flag in (("first", "-O2"), ("second", "-O2"), ("second", "-O0")):
                with self.subTest(compiler=name, option=flag):
                    result = build(name, flag)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(executable.read_text(), f"{name} {flag}\n")

            self.assertNotEqual(build("second", "-DFAIL").returncode, 0)
            self.assertEqual(executable.read_text(), "second -O0\n")
