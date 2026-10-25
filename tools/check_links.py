#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# ///
"""Check internal links and anchors in a built MkDocs site."""

from __future__ import annotations

import argparse
import html
import re
import sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SITE_DIR = ROOT / "site"
DEFAULT_CONFIG = ROOT / "mkdocs.yml"
LINK_ATTRS = {
    "a": ("href",),
    "link": ("href",),
    "script": ("src",),
    "img": ("src",),
    "iframe": ("src",),
    "source": ("src",),
}
SITE_URL = re.compile(r"^site_url:\s*(\S+)\s*$", re.MULTILINE)
ID = re.compile(r"""\b(?:id|name)\s*=\s*["']([^"']+)["']""")


class LinkParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.links: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        for name in LINK_ATTRS.get(tag, ()):
            for attr, value in attrs:
                if attr == name and value:
                    self.links.append(value)


def base_path(config: Path) -> str:
    if not config.is_file():
        return "/"
    match = SITE_URL.search(config.read_text(encoding="utf-8"))
    if match is None:
        return "/"
    return urlsplit(match.group(1)).path or "/"


def target_for(
    source: Path, raw: str, site_dir: Path, base: str
) -> tuple[Path, str] | None:
    url = urlsplit(html.unescape(raw))
    if url.scheme or url.netloc or raw.startswith("//"):
        return None
    if not url.path:
        return source, unquote(url.fragment)
    path = unquote(url.path)
    if path.startswith("/"):
        prefix = base if base.endswith("/") else base + "/"
        if not path.startswith(prefix):
            return site_dir / path.lstrip("/"), unquote(url.fragment)
        target = (site_dir / path[len(prefix) :]).resolve()
    else:
        target = (source.parent / path).resolve()
    if path.endswith("/") or target.is_dir():
        target = target / "index.html"
    return target, unquote(url.fragment)


def anchors(path: Path) -> set[str]:
    text = path.read_text(encoding="utf-8", errors="replace")
    return {html.unescape(value) for value in ID.findall(text)}


def check(site_dir: Path, base: str) -> tuple[int, list[str]]:
    checked = 0
    failures: list[str] = []
    seen: set[tuple[Path, str]] = set()
    for source in sorted(site_dir.rglob("*.html")):
        parser = LinkParser()
        parser.feed(source.read_text(encoding="utf-8", errors="replace"))
        for raw in parser.links:
            resolved = target_for(source, raw, site_dir, base)
            if resolved is None:
                continue
            target, fragment = resolved
            checked += 1
            key = (source.relative_to(site_dir), raw)
            if key in seen:
                continue
            seen.add(key)
            if not target.is_file():
                failures.append(f"{key[0]}: missing target {raw}")
                continue
            if fragment and target.suffix == ".html":
                if fragment not in anchors(target):
                    failures.append(f"{key[0]}: missing anchor {raw}")
    return checked, failures


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--site-dir", type=Path, default=DEFAULT_SITE_DIR)
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    args = parser.parse_args()

    if not args.site_dir.is_dir():
        print(f"missing site directory: {args.site_dir}", file=sys.stderr)
        return 1

    base = base_path(args.config)
    checked, failures = check(args.site_dir, base)
    if failures:
        for failure in failures:
            print(f"FAIL {failure}", file=sys.stderr)
        return 1
    print(f"{checked} internal links and anchors ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
