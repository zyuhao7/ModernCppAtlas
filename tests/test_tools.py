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

    def check(self, corpus_license="MIT", **overrides) -> list[str]:
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
            "license": corpus_license,
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

    def test_empty_excerpt_is_rejected_for_permissive_license(self) -> None:
        errors = self.check(excerpt="")
        self.assertEqual(len(errors), 1)
        self.assertIn("excerpt is empty but MIT allows quoting", errors[0])

    def test_empty_excerpt_is_allowed_when_license_forbids_quoting(self) -> None:
        errors = self.check(
            excerpt="", license="GPL-3.0-only", corpus_license="GPL-3.0-only"
        )
        self.assertEqual(errors, [])


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

    def test_published_evidence_requires_non_test_entry(self) -> None:
        report = self.validate.Report()
        self.validate.check_evidence_mix(
            report, self.feature_path, self.evidence(3, tests=True)
        )
        self.assertTrue(any("non-test" in error for error in report.errors))
        report = self.validate.Report()
        self.validate.check_evidence_mix(report, self.feature_path, self.evidence(1))
        self.assertEqual(report.errors, [])

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


class ClaimsTests(unittest.TestCase):
    def setUp(self) -> None:
        self.validate = load_tool("validate")
        self.path = Path("/tmp/cpp17.example.md")

    def check(self, tier: str, claims: list[dict] | None, evidence: list[dict]):
        report = self.validate.Report()
        self.validate.check_claims(
            report, self.path, {"tier": tier, "claims": claims}, evidence
        )
        return report.errors

    def evidence(self, *ids: str, level: str = "manual") -> list[dict]:
        return [{"id": evidence_id, "level": level} for evidence_id in ids]

    def claim(self, claim_id: str, *evidence_ids: str) -> dict:
        return {"id": claim_id, "evidence": list(evidence_ids)}

    def test_tier_a_needs_a_ledger(self) -> None:
        errors = self.check("A", None, self.evidence("ev-a"))
        self.assertTrue(any("claims ledger" in error for error in errors))

    def test_tier_b_may_omit_ledger(self) -> None:
        self.assertEqual(self.check("B", None, self.evidence("ev-a")), [])

    def test_claim_must_reference_published_evidence(self) -> None:
        errors = self.check(
            "A",
            [self.claim("c1", "ev-candidate")],
            self.evidence("ev-candidate", level="candidate"),
        )
        self.assertTrue(any("not published evidence" in error for error in errors))

    def test_orphan_published_evidence_is_rejected(self) -> None:
        errors = self.check(
            "A", [self.claim("c1", "ev-a")], self.evidence("ev-a", "ev-b")
        )
        self.assertTrue(any("not referenced by any claim" in error for error in errors))

    def test_duplicate_claim_ids_are_rejected(self) -> None:
        errors = self.check(
            "A",
            [self.claim("c1", "ev-a"), self.claim("c1", "ev-a")],
            self.evidence("ev-a"),
        )
        self.assertTrue(any("duplicate claim id" in error for error in errors))

    def test_valid_ledger_passes(self) -> None:
        errors = self.check(
            "A",
            [self.claim("c1", "ev-a"), self.claim("c2", "ev-b")],
            self.evidence("ev-a", "ev-b"),
        )
        self.assertEqual(errors, [])


class ReviewTests(unittest.TestCase):
    def setUp(self) -> None:
        self.validate = load_tool("validate")
        self.path = Path("/tmp/cpp17.example.md")

    def check(self, tier: str, review: dict | None) -> list[str]:
        report = self.validate.Report()
        self.validate.check_review(report, self.path, {"tier": tier, "review": review})
        return report.errors

    def review(self, level: str, *reviewers: str, author: str = "alice") -> dict:
        return {"author": author, "level": level, "reviewers": list(reviewers)}

    def test_tier_a_needs_a_review_block(self) -> None:
        errors = self.check("A", None)
        self.assertTrue(any("review block" in error for error in errors))

    def test_manual_review_allows_one_reviewer(self) -> None:
        self.assertEqual(self.check("B", self.review("manual", "alice")), [])

    def test_manual_review_needs_a_reviewer(self) -> None:
        errors = self.check("B", self.review("manual"))
        self.assertTrue(any("needs a reviewer" in error for error in errors))

    def test_verified_review_needs_two_reviewers(self) -> None:
        errors = self.check("B", self.review("verified", "alice"))
        self.assertTrue(any("two distinct reviewers" in error for error in errors))

    def test_verified_review_needs_a_non_author(self) -> None:
        errors = self.check("B", self.review("verified", "alice", "alice"))
        self.assertTrue(
            any("at least one non-author" in error for error in errors)
            or any("two distinct reviewers" in error for error in errors)
        )

    def test_verified_review_passes_with_non_author(self) -> None:
        self.assertEqual(self.check("A", self.review("verified", "alice", "bob")), [])


class EvidenceReviewTests(unittest.TestCase):
    def setUp(self) -> None:
        self.validate = load_tool("validate")
        self.path = Path("/tmp/ev-example.json")

    def check(self, **overrides) -> list[str]:
        evidence = {
            "level": "manual",
            "verified_by": ["alice"],
            "verified_at": "2026-10-25",
        }
        evidence.update(overrides)
        report = self.validate.Report()
        self.validate.check_evidence_review(report, self.path, evidence)
        return report.errors

    def test_manual_needs_reviewer_and_date(self) -> None:
        self.assertTrue(self.check(verified_by=[]))
        self.assertTrue(self.check(verified_at=None))
        self.assertEqual(self.check(), [])

    def test_verified_needs_two_reviewers_and_author(self) -> None:
        errors = self.check(
            level="verified", verified_by=["alice"], submitted_by="alice"
        )
        self.assertTrue(any("two reviewers" in error for error in errors))
        errors = self.check(
            level="verified", verified_by=["alice", "alice"], submitted_by="alice"
        )
        self.assertTrue(any("non-author" in error for error in errors))
        errors = self.check(level="verified", verified_by=["alice", "bob"])
        self.assertTrue(any("submitted_by" in error for error in errors))
        self.assertEqual(
            self.check(
                level="verified",
                verified_by=["alice", "bob"],
                submitted_by="alice",
            ),
            [],
        )

    def test_candidate_must_not_list_reviewers(self) -> None:
        self.assertTrue(self.check(level="candidate", verified_at=None))
        self.assertEqual(
            self.check(level="candidate", verified_by=[], verified_at=None), []
        )


class ClaimTableTests(unittest.TestCase):
    def test_renders_evidence_links_and_escapes_pipes(self) -> None:
        build_site = load_tool("build_site")
        table = build_site.claim_table(
            {
                "claims": [
                    {
                        "id": "c1",
                        "statement": "读 | 写",
                        "evidence": ["ev-one", "ev-two"],
                    }
                ]
            }
        )
        self.assertIn("读 \\| 写", table)
        self.assertIn("[ev-one](../evidence/ev-one.md)", table)
        self.assertIn("[ev-two](../evidence/ev-two.md)", table)

    def test_missing_ledger_renders_nothing(self) -> None:
        build_site = load_tool("build_site")
        self.assertEqual(build_site.claim_table({}), "")


class LibraryCardTests(unittest.TestCase):
    def test_loads_cards_from_directory(self) -> None:
        build_site = load_tool("build_site")
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            (directory / "lib-one.yaml").write_text("id: lib-one\n", encoding="utf-8")
            self.assertEqual(build_site.load_libraries(directory), [{"id": "lib-one"}])

    def test_renders_feature_evidence_and_example_links(self) -> None:
        build_site = load_tool("build_site")
        with tempfile.TemporaryDirectory() as tmp:
            build_site.OUT_DIR = Path(tmp)
            build_site.build_libraries_page(
                {"cpp20.span": {"title": "std::span"}},
                [
                    {
                        "id": "lib-x",
                        "library": "owner/repo",
                        "feature": "cpp20.span",
                        "pattern": "p",
                        "benefit": "b",
                        "cost": "c",
                        "evidence": ["ev-x"],
                        "example": "examples/cpp20/span.cpp",
                    }
                ],
                "https://example.test/repo",
                "main",
            )
            text = (Path(tmp) / "libraries" / "index.md").read_text(encoding="utf-8")
        self.assertIn("../features/cpp20.span.md", text)
        self.assertIn("../evidence/ev-x.md", text)
        self.assertIn(
            "https://example.test/repo/blob/main/examples/cpp20/span.cpp", text
        )

    def test_skips_page_without_cards(self) -> None:
        build_site = load_tool("build_site")
        with tempfile.TemporaryDirectory() as tmp:
            build_site.OUT_DIR = Path(tmp)
            build_site.build_libraries_page({}, [], "https://example.test", "main")
            self.assertFalse((Path(tmp) / "libraries" / "index.md").exists())


class GeneratedEvidenceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.validate = load_tool("validate")
        self.path = Path("/tmp/ev-example.json")

    def check(self, **overrides) -> list[str]:
        evidence = {"level": "manual", "generated": False}
        evidence.update(overrides)
        report = self.validate.Report()
        self.validate.check_generated_evidence(report, self.path, evidence)
        return report.errors

    def exception(self, approver: str = "bob") -> dict:
        return {
            "approved_by": approver,
            "approved_at": "2026-10-25",
            "rationale": "生成文件是用户实际使用的唯一形式。",
        }

    def test_generated_review_requires_flag(self) -> None:
        errors = self.check(generated_review=self.exception())
        self.assertTrue(any("requires generated: true" in error for error in errors))

    def test_generated_published_evidence_needs_exception(self) -> None:
        errors = self.check(generated=True, submitted_by="alice")
        self.assertTrue(any("rejected by default" in error for error in errors))

    def test_generated_exception_needs_submitter(self) -> None:
        errors = self.check(generated=True, generated_review=self.exception())
        self.assertTrue(any("needs submitted_by" in error for error in errors))

    def test_generated_exception_needs_independent_approver(self) -> None:
        errors = self.check(
            generated=True,
            submitted_by="alice",
            generated_review=self.exception("alice"),
        )
        self.assertTrue(any("not the submitter" in error for error in errors))

    def test_generated_exception_with_independent_approver_passes(self) -> None:
        self.assertEqual(
            self.check(
                generated=True,
                submitted_by="alice",
                generated_review=self.exception("bob"),
            ),
            [],
        )

    def test_unpublished_generated_candidate_needs_no_exception(self) -> None:
        self.assertEqual(self.check(level="candidate", generated=True), [])


class LinkCheckTests(unittest.TestCase):
    def setUp(self) -> None:
        self.links = load_tool("check_links")
        self.tmp = tempfile.TemporaryDirectory()
        self.site = Path(self.tmp.name) / "site"
        self.site.mkdir()
        self.config = Path(self.tmp.name) / "mkdocs.yml"
        self.config.write_text(
            "site_url: https://example.com/Atlas/\n", encoding="utf-8"
        )

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def write(self, name: str, body: str) -> Path:
        path = self.site / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(body, encoding="utf-8")
        return path

    def test_relative_directory_link_and_anchor(self) -> None:
        self.write("guide/index.html", '<h2 id="intro">intro</h2>')
        source = self.write(
            "index.html",
            '<a href="guide/#intro">guide</a>'
            '<a href="https://example.com">external</a>',
        )
        base = self.links.base_path(self.config)
        checked, failures = self.links.check(self.site, base)
        self.assertEqual(failures, [])
        self.assertEqual(checked, 1)
        target, fragment = self.links.target_for(
            source, "guide/#intro", self.site, base
        )
        self.assertEqual(target, self.site / "guide" / "index.html")
        self.assertEqual(fragment, "intro")

    def test_missing_target_and_anchor_are_reported(self) -> None:
        self.write("guide/index.html", "<p>no anchors</p>")
        self.write(
            "index.html",
            '<a href="missing/">x</a><a href="guide/#gone">y</a>',
        )
        _, failures = self.links.check(self.site, "/")
        self.assertTrue(any("missing target" in item for item in failures))
        self.assertTrue(any("missing anchor" in item for item in failures))

    def test_root_relative_link_uses_site_base(self) -> None:
        self.write("features/index.html", "<p>ok</p>")
        self.write("index.html", '<a href="/Atlas/features/">features</a>')
        base = self.links.base_path(self.config)
        self.assertEqual(base, "/Atlas/")
        self.assertEqual(self.links.check(self.site, base)[1], [])

    def test_root_relative_outside_base_is_broken(self) -> None:
        self.write("index.html", '<a href="/elsewhere/">elsewhere</a>')
        base = self.links.base_path(self.config)
        failures = self.links.check(self.site, base)[1]
        self.assertTrue(any("missing target" in item for item in failures))


class CompilerTableTests(unittest.TestCase):
    def setUp(self) -> None:
        self.validate = load_tool("validate")
        self.feature = {
            "standard": "C++17",
            "compilers": {"gcc": "7", "clang": "4", "msvc": "19.11"},
        }
        self.entry = {
            "gcc": "7",
            "clang": "4",
            "msvc": "19.11",
            "source": "https://en.cppreference.com/w/cpp/compiler_support/17",
            "checked_at": "2026-10-26",
        }

    def check(self, features, entries) -> list[str]:
        report = self.validate.Report()
        self.validate.check_compilers(report, features, {"features": entries})
        return report.errors

    def test_matching_table_passes(self) -> None:
        errors = self.check({"cpp17.any": self.feature}, {"cpp17.any": self.entry})
        self.assertEqual(errors, [])

    def test_missing_and_unknown_entries_are_reported(self) -> None:
        errors = self.check({"cpp17.any": self.feature}, {"cpp17.other": self.entry})
        self.assertTrue(any("missing compiler entry" in error for error in errors))
        self.assertTrue(any("unknown feature" in error for error in errors))

    def test_version_mismatch_is_reported(self) -> None:
        entry = dict(self.entry, gcc="8")
        errors = self.check({"cpp17.any": self.feature}, {"cpp17.any": entry})
        self.assertTrue(any("gcc '7' differs" in error for error in errors))

    def test_source_must_match_standard(self) -> None:
        entry = dict(
            self.entry, source="https://en.cppreference.com/w/cpp/compiler_support/20"
        )
        errors = self.check({"cpp17.any": self.feature}, {"cpp17.any": entry})
        self.assertTrue(any("C++17 table" in error for error in errors))

    def test_invalid_checked_at_is_reported(self) -> None:
        entry = dict(self.entry, checked_at="2026-02-31")
        errors = self.check({"cpp17.any": self.feature}, {"cpp17.any": entry})
        self.assertTrue(any("not a valid date" in error for error in errors))


class IdentityCheckTests(unittest.TestCase):
    def setUp(self) -> None:
        self.validate = load_tool("validate")
        self.path = Path("/tmp/cpp17.example.md")

    def test_standard_must_match_id(self) -> None:
        report = self.validate.Report()
        self.validate.check_standard(report, self.path, "cpp17.example", "C++20")
        self.assertTrue(any("does not match id" in error for error in report.errors))

    def test_matching_standard_passes(self) -> None:
        report = self.validate.Report()
        self.validate.check_standard(report, self.path, "cpp20.example", "C++20")
        self.assertEqual(report.errors, [])

    def test_invalid_calendar_date_is_reported(self) -> None:
        report = self.validate.Report()
        self.validate.check_date(report, self.path, "last_verified", "2026-02-31")
        self.assertTrue(any("not a valid date" in error for error in report.errors))

    def test_valid_or_missing_date_passes(self) -> None:
        report = self.validate.Report()
        self.validate.check_date(report, self.path, "last_verified", "2026-10-25")
        self.validate.check_date(report, self.path, "verified_at", None)
        self.assertEqual(report.errors, [])


class ReportTests(unittest.TestCase):
    def test_path_outside_root_does_not_crash(self) -> None:
        validate = load_tool("validate")
        report = validate.Report()
        report.error(Path("/tmp/outside.json"), "problem")
        self.assertIn("problem", report.errors[0])


class FreshnessTests(unittest.TestCase):
    SHA = "a" * 40
    EVIDENCE = {
        "id": "ev-owner-example-001",
        "repo": "owner/repo",
        "commit": SHA,
        "path": "dir/file.h",
        "lines": "1-2",
        "feature": "cpp17.example",
        "license": "MIT",
    }

    def setUp(self) -> None:
        self.freshness = load_tool("check_evidence_freshness")
        self.original_fetch = self.freshness.fetch_json

    def tearDown(self) -> None:
        self.freshness.fetch_json = self.original_fetch

    def check(self, **overrides):
        responses = {
            "https://api.github.com/repos/owner/repo": (200, {}),
            f"https://api.github.com/repos/owner/repo/commits/{self.SHA}": (200, {}),
            "https://api.github.com/repos/owner/repo/contents/dir/file.h"
            f"?ref={self.SHA}": (200, {"type": "file"}),
            "https://api.github.com/repos/owner/repo/license": (
                200,
                {"license": {"spdx_id": "MIT"}},
            ),
        }
        responses.update(overrides)

        def fetch(url, _token, _timeout=20.0):
            result = responses[url]
            if isinstance(result, Exception):
                raise result
            return result

        self.freshness.fetch_json = fetch
        return self.freshness.Checker("token").check(dict(self.EVIDENCE))

    def test_resolving_evidence_is_ok(self) -> None:
        self.assertEqual(self.check(), ("ok", ""))

    def test_missing_repository_is_structural(self) -> None:
        status, _ = self.check(**{"https://api.github.com/repos/owner/repo": (404, {})})
        self.assertEqual(status, "repo_missing")

    def test_missing_commit_is_structural(self) -> None:
        url = f"https://api.github.com/repos/owner/repo/commits/{self.SHA}"
        self.assertEqual(self.check(**{url: (422, {})})[0], "commit_missing")

    def test_missing_path_is_structural(self) -> None:
        url = (
            "https://api.github.com/repos/owner/repo/contents/dir/file.h"
            f"?ref={self.SHA}"
        )
        self.assertEqual(self.check(**{url: (404, {})})[0], "path_missing")

    def test_changed_license_is_structural(self) -> None:
        url = "https://api.github.com/repos/owner/repo/license"
        status, detail = self.check(
            **{url: (200, {"license": {"spdx_id": "GPL-3.0-only"}})}
        )
        self.assertEqual(status, "license_changed")
        self.assertIn("GPL-3.0-only", detail)

    def test_network_failure_is_unknown(self) -> None:
        url = "https://api.github.com/repos/owner/repo"
        status, _ = self.check(**{url: self.freshness.NetworkError("reset")})
        self.assertEqual(status, "unknown")

    def test_apply_stale_preserves_unrelated_formatting(self) -> None:
        data = dict(self.EVIDENCE) | {
            "verified_by": ["reviewer"],
            "level": "manual",
        }
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "evidence.json"
            path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n")
            feature = self.freshness.apply_stale(
                path,
                {"status": "repo_missing", "detail": "repository not found"},
                "2026-10-28",
            )
            text = path.read_text()
            updated = json.loads(text)
        self.assertEqual(feature, "cpp17.example")
        self.assertEqual(updated["level"], "stale")
        self.assertEqual(updated["stale_since"], "2026-10-28")
        self.assertIn("repo_missing", updated["stale_reason"])
        self.assertIn('"verified_by": [\n    "reviewer"\n  ]', text)

    def test_apply_stale_keeps_inline_array_style(self) -> None:
        data = dict(self.EVIDENCE) | {
            "verified_by": ["reviewer"],
            "level": "manual",
        }
        original = (
            json.dumps(data, indent=2, ensure_ascii=False).replace(
                '"verified_by": [\n    "reviewer"\n  ]', '"verified_by": ["reviewer"]'
            )
            + "\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "evidence.json"
            path.write_text(original)
            self.freshness.apply_stale(
                path, {"status": "commit_missing", "detail": "gone"}, "2026-10-28"
            )
            text = path.read_text()
        self.assertIn('"verified_by": ["reviewer"]', text)


class StaleValidationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.validate = load_tool("validate")
        self.path = Path("/tmp/evidence.json")
        self.evidence = {
            "level": "stale",
            "stale_reason": "repo_missing: repository not found",
            "stale_since": "2026-10-28",
            "verified_by": ["reviewer"],
            "verified_at": "2026-10-28",
        }

    def check(self, **overrides) -> list[str]:
        report = self.validate.Report()
        evidence = self.evidence | overrides
        self.validate.check_evidence_review(report, self.path, evidence)
        return report.errors

    def test_stale_evidence_needs_reason_and_date(self) -> None:
        self.assertEqual(self.check(), [])
        self.assertTrue(self.check(stale_reason=" "))
        self.assertTrue(self.check(stale_since=None))

    def test_stale_fields_only_valid_for_stale_level(self) -> None:
        errors = self.check(level="manual")
        self.assertTrue(any("only valid for stale" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
