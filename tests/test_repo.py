from __future__ import annotations

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LINK = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
EXTERNAL_PREFIXES = ("http://", "https://", "mailto:", "#")


class RepositoryDocsTests(unittest.TestCase):
    def test_license_files_exist(self) -> None:
        code = ROOT / "LICENSE"
        content = ROOT / "LICENSE-CONTENT"
        self.assertTrue(code.is_file())
        self.assertTrue(content.is_file())
        self.assertIn("MIT License", code.read_text(encoding="utf-8"))
        self.assertIn(
            "Attribution 4.0 International", content.read_text(encoding="utf-8")
        )

    def test_readme_local_links_exist(self) -> None:
        readme = ROOT / "README.md"
        for target in LINK.findall(readme.read_text(encoding="utf-8")):
            target = target.strip()
            if target.startswith(EXTERNAL_PREFIXES):
                continue
            relative = target.split("#", 1)[0].strip("<>")
            if not relative:
                continue
            with self.subTest(target=target):
                self.assertTrue((readme.parent / relative).exists(), target)


if __name__ == "__main__":
    unittest.main()
