#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.12"
# dependencies = ["pyyaml>=6.0"]
# ///
"""Check internal links and anchors in a built MkDocs site."""

from __future__ import annotations

import argparse
import sys
from functools import cache
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

import yaml

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


class LinkParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.links: list[str] = []
        self.anchors: set[str] = set()

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        link_attrs = LINK_ATTRS.get(tag, ())
        for attr, value in attrs:
            if not value:
                continue
            if attr in link_attrs:
                self.links.append(value)
            if attr == "id" or (tag == "a" and attr == "name"):
                self.anchors.add(value)


def base_path(config: Path) -> str:
    if not config.is_file():
        return "/"
    options = yaml.safe_load(config.read_text(encoding="utf-8")) or {}
    site_url = options.get("site_url")
    if not site_url:
        return "/"
    return unquote(urlsplit(site_url).path) or "/"


def target_for(
    source: Path, raw: str, site_dir: Path, base: str
) -> tuple[Path | None, str] | None:
    url = urlsplit(raw)
    if url.scheme or url.netloc or raw.startswith("//"):
        return None
    fragment = unquote(url.fragment)
    if not url.path:
        return source, fragment
    path = unquote(url.path)
    if path.startswith("/"):
        prefix = base.rstrip("/") + "/"
        if path == prefix.removesuffix("/"):
            target = site_dir
        elif path.startswith(prefix):
            target = site_dir / path.removeprefix(prefix)
        else:
            return None, fragment
    else:
        target = source.parent / path
    target = target.resolve()
    if not target.is_relative_to(site_dir.resolve()):
        return None, fragment
    if path.endswith("/") or target.is_dir():
        target = target / "index.html"
    return target, fragment


def check(site_dir: Path, base: str) -> tuple[int, list[str]]:
    @cache
    def parse_page(path: Path) -> LinkParser:
        parser = LinkParser()
        parser.feed(path.read_text(encoding="utf-8", errors="replace"))
        parser.close()
        return parser

    site_dir = site_dir.resolve()
    checked = 0
    failures: list[str] = []
    seen: set[tuple[Path, str]] = set()
    for source in sorted(site_dir.rglob("*.html")):
        for raw in parse_page(source).links:
            resolved = target_for(source, raw, site_dir, base)
            if resolved is None:
                continue
            target, fragment = resolved
            checked += 1
            key = (source.relative_to(site_dir), raw)
            if key in seen:
                continue
            seen.add(key)
            if target is None or not target.is_file():
                failures.append(f"{key[0]}: missing target {raw}")
                continue
            if fragment and target.suffix == ".html":
                if fragment not in parse_page(target).anchors:
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
