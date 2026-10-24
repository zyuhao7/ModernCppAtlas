from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load_tool(name: str):
    path = ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(f"atlas_{name}", path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def git(repo: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", *args], cwd=repo, text=True, capture_output=True, check=True
    )
    return result.stdout.strip()


class OutputDirectoryTests(unittest.TestCase):
    def setUp(self) -> None:
        self.build_site = load_tool("build_site")
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        for name in self.build_site.SOURCE_DIRS:
            (self.root / name).mkdir()

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def test_rejects_source_directory_and_descendants(self) -> None:
        paths = (
            self.root / "data",
            self.root / "data" / "features",
            self.root / "tests",
        )
        for path in paths:
            with self.subTest(path=path):
                with self.assertRaises(SystemExit):
                    self.build_site.prepare_output(path, self.root)

    def test_rejects_root_and_parent(self) -> None:
        for path in (self.root, self.root.parent):
            with self.subTest(path=path):
                with self.assertRaises(SystemExit):
                    self.build_site.prepare_output(path, self.root)

    def test_rejects_unmarked_nonempty_directory(self) -> None:
        output = self.root / "out"
        output.mkdir()
        (output / "keep.txt").write_text("user data\n")
        with self.assertRaises(SystemExit):
            self.build_site.prepare_output(output, self.root)
        self.assertTrue((output / "keep.txt").exists())

    def test_marked_directory_is_cleaned(self) -> None:
        output = self.root / "out"
        output.mkdir()
        (output / "old.txt").write_text("old\n")
        (output / self.build_site.GENERATED_MARKER).write_text("generated\n")
        target = self.build_site.prepare_output(output, self.root)
        self.assertEqual(target, output.resolve())
        self.assertFalse((output / "old.txt").exists())
        self.assertTrue((output / self.build_site.GENERATED_MARKER).is_file())

    def test_rejects_symlink(self) -> None:
        target = self.root / "out"
        target.mkdir()
        link = self.root / "link"
        link.symlink_to(target)
        with self.assertRaises(SystemExit):
            self.build_site.prepare_output(link, self.root)


class EvidenceFilterTests(unittest.TestCase):
    def test_only_published_levels_are_loaded(self) -> None:
        build_site = load_tool("build_site")
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            for index, level in enumerate(["manual", "verified", "candidate", "stale"]):
                record = {"feature": "cpp17.example", "level": level, "id": str(index)}
                (directory / f"{index}.json").write_text(json.dumps(record))
            grouped = build_site.load_evidence(directory)
        records = grouped["cpp17.example"]
        self.assertEqual(
            {record["level"] for record in records}, {"manual", "verified"}
        )
        self.assertEqual(len(records), 2)


class CorpusLinkTests(unittest.TestCase):
    def setUp(self) -> None:
        self.validate = load_tool("validate")
        self.tmp = tempfile.TemporaryDirectory()
        base = Path(self.tmp.name)
        self.cache = base / "corpus"
        self.repo = self.cache / "owner__repo"
        self.repo.mkdir(parents=True)
        git(self.repo, "init")
        git(self.repo, "config", "user.email", "test@example.com")
        git(self.repo, "config", "user.name", "Test")
        self.source = self.repo / "src.cpp"
        self.source.write_text("line1\nline2\nline3\n")
        git(self.repo, "add", "src.cpp")
        git(self.repo, "commit", "-m", "first")
        self.first = git(self.repo, "rev-parse", "HEAD")
        self.source.write_text("line1\nchanged\nline3\n")
        git(self.repo, "add", "src.cpp")
        git(self.repo, "commit", "-m", "second")
        self.second = git(self.repo, "rev-parse", "HEAD")

        self.original_cache = self.validate.CORPUS_CACHE
        self.validate.CORPUS_CACHE = self.cache

    def tearDown(self) -> None:
        self.validate.CORPUS_CACHE = self.original_cache
        self.tmp.cleanup()

    def check(self, **overrides) -> list[str]:
        evidence = {
            "repo": "owner/repo",
            "commit": self.first,
            "path": "src.cpp",
            "lines": "1-2",
            "excerpt": "line1\nline2",
            "license": "MIT",
        }
        evidence.update(overrides)
        entry = {
            "repo": "owner/repo",
            "commit": self.second,
            "license": "MIT",
        }
        report = self.validate.Report()
        self.validate.check_corpus_link(
            report, Path("/tmp/evidence.json"), evidence, {"owner/repo": entry}
        )
        return report.errors

    def test_historical_commit_is_validated_at_its_own_revision(self) -> None:
        self.assertNotEqual(self.first, self.second)
        self.assertEqual(self.check(), [])

    def test_excerpt_is_compared_with_source(self) -> None:
        errors = self.check(excerpt="line1\nnot line2")
        self.assertEqual(len(errors), 1)
        self.assertIn("excerpt line 2 differs from source", errors[0])

    def test_license_must_match_corpus_declaration(self) -> None:
        errors = self.check(license="Apache-2.0")
        self.assertEqual(len(errors), 1)
        self.assertIn("differs from corpus declaration", errors[0])

    def test_unknown_commit_is_reported(self) -> None:
        errors = self.check(commit="0" * 40)
        self.assertEqual(len(errors), 1)
        self.assertIn("is not available in the cache", errors[0])


class TierTests(unittest.TestCase):
    HEADINGS = {
        "A": [
            "解决什么问题",
            "基础用法",
            "进阶用法",
            "真实项目证据",
            "陷阱与边界",
            "库联动",
            "横向对比",
        ],
        "B": ["解决什么问题", "基础用法", "真实项目证据", "陷阱与边界", "横向对比"],
    }

    def setUp(self) -> None:
        self.validate = load_tool("validate")
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.feature_dir = self.root / "data" / "features"
        self.feature_dir.mkdir(parents=True)
        self.feature_path = self.feature_dir / "cpp17.example.md"

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def evidence(self, count: int, *, tests: bool = False) -> list[dict]:
        return [{"level": "manual", "is_test": tests} for _ in range(count)]

    def body(self, tier: str, *, omit: str | None = None, example: str | None = None):
        headings = [heading for heading in self.HEADINGS[tier] if heading != omit]
        parts = [f"## {heading}\n\ntext\n" for heading in headings]
        if example:
            parts.append(f"[example](../../examples/{example})\n")
        return "\n".join(parts)

    def check(self, tier: str, body: str, evidence: list[dict]) -> list[str]:
        report = self.validate.Report()
        self.validate.check_tier(
            report, self.feature_path, {"tier": tier}, body, evidence
        )
        return report.errors

    def make_example(self, relative: str) -> None:
        path = self.root / "examples" / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("int main() {}\n")

    def test_valid_tier_a(self) -> None:
        self.make_example("cpp17/example.cpp")
        body = self.body("A", example="cpp17/example.cpp")
        self.assertEqual(self.check("A", body, self.evidence(3)), [])

    def test_tier_a_requires_three_published_entries(self) -> None:
        self.make_example("cpp17/example.cpp")
        body = self.body("A", example="cpp17/example.cpp")
        errors = self.check("A", body, self.evidence(2))
        self.assertTrue(any("at least 3" in error for error in errors))

    def test_tier_a_requires_production_evidence(self) -> None:
        self.make_example("cpp17/example.cpp")
        body = self.body("A", example="cpp17/example.cpp")
        errors = self.check("A", body, self.evidence(3, tests=True))
        self.assertTrue(any("non-test" in error for error in errors))

    def test_tier_a_requires_sections_and_example(self) -> None:
        body = self.body("A", omit="横向对比")
        errors = self.check("A", body, self.evidence(3))
        self.assertTrue(any("missing sections: 横向对比" in error for error in errors))
        self.assertTrue(any("needs a compilable example" in error for error in errors))

    def test_tier_a_rejects_missing_example_file(self) -> None:
        body = self.body("A", example="cpp17/missing.cpp")
        errors = self.check("A", body, self.evidence(3))
        self.assertTrue(any("example link does not exist" in error for error in errors))

    def test_valid_tier_b(self) -> None:
        self.assertEqual(self.check("B", self.body("B"), self.evidence(2)), [])

    def test_tier_c_requires_published_evidence(self) -> None:
        candidate = [{"level": "candidate", "is_test": False}]
        errors = self.check("C", "", candidate)
        self.assertTrue(any("at least 1" in error for error in errors))


class ReportTests(unittest.TestCase):
    def test_path_outside_root_does_not_crash(self) -> None:
        validate = load_tool("validate")
        report = validate.Report()
        report.error(Path("/tmp/outside.json"), "problem")
        self.assertIn("problem", report.errors[0])


if __name__ == "__main__":
    unittest.main()
