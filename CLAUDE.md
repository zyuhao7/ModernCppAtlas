# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repository is

A Chinese-language, data-driven knowledge base for modern C++ (C++11 through C++26). It is not a C++ library or application: the product is a set of Markdown feature pages backed by pinned real-world evidence, validated by Python tools and rendered to a MkDocs site. Content lives under `data/`; `tools/` enforces the data contract; `examples/` holds compilable programs referenced by feature pages.

## Common commands

```bash
uv sync                        # runtime tools
uv sync --group dev            # ruff, clang-format, mdformat
uv sync --extra docs           # mkdocs-material, jieba
```

Examples:

```bash
make examples                  # compile examples/cppNN/*.cpp with the matching -std
make run                       # compile and run each example
make verify                    # compile, run, and diff stdout against examples/expected/
CXX=clang++ make examples      # switch compiler
```

Validation pipeline:

```bash
uv run tools/fetch_corpus.py                  # shallow-clone pinned repos into .cache/corpus
uv run tools/validate.py                      # schema + cross-file checks, no network
uv run tools/validate.py --check-corpus       # also verify commits/paths/lines/excerpts
uv run tools/verify_examples.py --compiler g++ --run
uv run tools/check_snippets.py --compiler g++
uv run python -m unittest discover -s tests -v
```

Run a single test:

```bash
uv run python -m unittest tests.test_tools.CorpusLinkTests.test_excerpt_is_compared_with_source -v
```

Site build (order matters):

```bash
uv run tools/build_site.py
uv run --extra docs mkdocs build --strict
uv run tools/check_links.py
uv run tools/check_search.py
```

Formatting (must match the CI `format` job):

```bash
uv run ruff format tools tests && uv run ruff check tools tests
uv run clang-format -i examples/*/*.cpp
uv run mdformat README.md todo.md data/features .github/pull_request_template.md
```

Maintenance / network-dependent tools:

```bash
uv run tools/check_evidence_freshness.py --report .cache/freshness.json
uv run tools/check_evidence_freshness.py --apply-stale   # mutates evidence JSON
uv run tools/scan/scan.py                                # candidate discovery only
```

`verify_examples.py --update-expected` rewrites snapshots; treat it as a deliberate content change and explain it in the PR.

## Architecture

The data pipeline is:

```text
data/features/*.md ──┐
data/evidence/*.json ─┼─> schema/*.json + tools/validate.py ─> tools/build_site.py ─> .site-docs/ ─> mkdocs
data/libraries/*.yaml ┘
data/compilers.yaml ──┘
data/corpus.yaml ─────────> tools/fetch_corpus.py ─> .cache/corpus (git checkouts)
examples/ + examples/expected/ ─> tools/verify_examples.py ─> CI
```

### Feature pages (`data/features/<id>.md`)

YAML frontmatter followed by Chinese prose. `id` must equal the filename stem and its prefix must match `standard` (`C++20` → `cpp20.`). Sections are fixed per tier: A needs seven headings, B five, C none. Tier rules (`min evidence`, required headings, example requirement) are in `tools/validate.py`'s `TIER_RULES`. Tier A additionally requires a `claims` ledger and a `review` block.

### Evidence (`data/evidence/<id>.json`)

Each record is a pinned third-party usage:

- `commit` is a full 40-hex SHA; links must never point at a branch or tag.
- `lines` is `start-end`; `excerpt` must equal the source lines at that exact commit (checked only with `--check-corpus`), at most 15 lines and 1200 characters.
- Licenses in `EXCERPT_LICENSES` (MIT, Apache-2.0, BSD, BSL-1.0, CC0) may quote code; MPL/GPL/AGPL/LGPL entries must leave `excerpt` empty and record location only.
- Levels: `verified` (two distinct reviewers, one not the author/`submitted_by`), `manual` (one reviewer + `verified_at`), `candidate` (no reviewers), `stale` (requires `stale_reason` and `stale_since`). A single-maintainer project defaults to `manual`; never upgrade without genuine second review.
- Generated code is rejected by default unless `"generated": true`, `submitted_by`, and a `generated_review` with an independent approver and rationale.
- At least one published evidence entry per feature must be non-test code.

### Claims ledger

Feature frontmatter maps testable statements to evidence IDs. For tier A the ledger is mandatory. Every referenced ID must belong to that feature and be published; every published evidence entry must be referenced by at least one claim — orphan evidence fails validation. When evidence is demoted to `stale`, update the ledger and review block in the same change.

### Supporting data

- `data/libraries/*.yaml`: library-integration cards referencing one feature, several evidence IDs, and optionally an example path.
- `data/compilers.yaml`: minimum GCC/Clang/MSVC versions; must exactly mirror each feature's frontmatter `compilers` and point `source` at the matching cppreference standard table.
- `data/corpus.yaml`: repos, pinned commits and tags used by evidence and scanning.
- `data/taxonomy.yaml`: canonical `category` values and the presentation order of the scenario index; the schema enum mirrors it and `validate.py` rejects drift.
- `schema/*.json`: JSON Schema Draft 2020-12, loaded by `validate.py`.

### Site generation

`tools/build_site.py` reads features, published evidence (`manual`/`verified` only), and library cards, and writes Markdown into `.site-docs/`. It rewrites `../../examples/...` links to GitHub blob URLs.

## Gotchas

- **Pipeline ordering.** `fetch_corpus.py` before `validate.py --check-corpus` or `scan.py`; `build_site.py` before `mkdocs`, `check_links.py`, or `check_search.py`.
- **Destructive output directory.** `build_site.py` only deletes a non-empty `.site-docs/` carrying its `.moderncppatlas-generated` marker; it refuses symlinks and source directories. Do not remove the marker manually.
- **Underscore files diverge.** `validate.py` and `check_snippets.py` skip `_`-prefixed feature files; `build_site.py` does not. A helper Markdown file without a valid `id` will crash the site build.
- **Snippet markers.** A `cpp` fence is compiled only when the immediately preceding non-empty line is `<!-- compile -->`. For tiers A/B, the first `cpp` fence under `## 基础用法` must carry the marker. Snippets are checked with `-fsyntax-only`, not executed.
- **Example naming.** The standard comes from the `examples/cppNN/` directory, not the filename; the output snapshot is `examples/expected/cppNN/<name>.out`.
- **Freshness is advisory.** `check_evidence_freshness.py` writes a report and exits non-zero on structural failures (`repo_missing`, `commit_missing`, `path_missing`, `license_changed`); network/rate-limit failures are `unknown` and never demote evidence. Only `--apply-stale` mutates data. The monthly workflow opens/updates an issue instead of changing evidence automatically.
- **Chinese content.** Required section headings are exact strings compared by validation; do not translate or rename them.
- **Corpus cache.** `fetch_corpus.py` refuses to update a dirty checkout under `.cache/corpus`.

## CI

`.github/workflows/ci.yml` runs four independent jobs: `format` (ruff, clang-format, mdformat), `validate` (schemas then pinned-corpus checks), `tests` (unittest), and `examples` (GCC and Clang matrix, including snippet compilation and `make examples`). The `site` job generates sources, builds with `--strict`, checks links and Chinese search, and deploys to GitHub Pages on `main`. `monthly.yml` runs candidate scanning and evidence freshness checks.
