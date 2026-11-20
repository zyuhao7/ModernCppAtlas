from __future__ import annotations

import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
from unittest.mock import patch

import yaml

from tools import build_site, check_links


class ProjectLinkTests(unittest.TestCase):
    def test_root_asset_cannot_match_a_file_inside_the_project(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            site = Path(temporary)
            (site / "theme.css").touch()
            (site / "index.html").write_text(
                '<link rel="stylesheet" href="/theme.css">'
                '<link rel="stylesheet" href="/Atlas/theme.css">',
                encoding="utf-8",
            )

            _, failures = check_links.check(site, "/Atlas/")

        self.assertEqual(failures, ["index.html: missing target /theme.css"])

    def test_fragments_match_ids_and_legacy_named_anchors(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            site = Path(temporary)
            (site / "index.html").write_text(
                '<a href="guide.html#unquoted">real id</a>'
                '<a href="guide.html#legacy">named anchor</a>'
                '<a href="guide.html#encoded%26value">decoded id</a>'
                '<a href="guide.html#attribute">data attribute</a>'
                '<a href="guide.html#control">form control name</a>',
                encoding="utf-8",
            )
            (site / "guide.html").write_text(
                "<h2 id=unquoted>Guide</h2>"
                '<a name="legacy"></a>'
                '<span id="encoded&amp;value"></span>'
                '<div data-id="attribute"></div>'
                '<input name="control">',
                encoding="utf-8",
            )

            _, failures = check_links.check(site, "/Atlas/")

        self.assertEqual(
            failures,
            [
                "index.html: missing anchor guide.html#attribute",
                "index.html: missing anchor guide.html#control",
            ],
        )


class CompilerProvenanceTests(unittest.TestCase):
    def test_generated_feature_uses_recorded_source_and_date(self) -> None:
        feature_id = "cpp17.launder"
        compiler_table = yaml.safe_load(
            (build_site.ROOT / "data" / "compilers.yaml").read_text(encoding="utf-8")
        )
        compiler = compiler_table["features"][feature_id]
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "docs"
            with (
                patch.object(sys, "argv", ["build_site.py", "--output", str(output)]),
                patch.object(build_site, "OUT_DIR", output),
                redirect_stdout(StringIO()),
            ):
                self.assertEqual(build_site.main(), 0)
            page = (output / "features" / f"{feature_id}.md").read_text(
                encoding="utf-8"
            )

        metadata = next(
            line for line in page.splitlines() if line.startswith("编译器支持：")
        )
        self.assertIn(f"[cppreference 编译器支持]({compiler['source']})", metadata)
        self.assertIn(f"查阅日期 {compiler['checked_at']}", metadata)
