---
name: verify
description: Run the full ModernCppAtlas validation chain (data contracts, pinned evidence, snippets, examples, tests, site, formatting) in CI order. Use when asked to verify, check, or prepare work before a commit or PR.
---

Run steps in order and stop at the first failure. Do not fix anything unless asked — report the failing step and the exact error.

1. Data contracts: `uv run tools/validate.py`
2. Pinned evidence: `uv run tools/validate.py --check-corpus`
   - Requires `.cache/corpus/`. If missing and network is unavailable, report that this step was skipped rather than faking it.
3. Article snippets: `uv run tools/check_snippets.py --compiler g++`
4. Examples: `uv run tools/verify_examples.py --compiler g++ --run`
   - Do not pass `--update-expected` here; it rewrites snapshots.
5. Tool tests: `uv run python -m unittest discover -s tests -v`
6. Site: `uv run tools/build_site.py && uv run --extra docs mkdocs build --strict`
7. Links: `uv run tools/check_links.py`
8. Search index: `uv run tools/check_search.py`
9. Formatting (check only):
   - `uv run ruff format --check tools tests && uv run ruff check tools tests`
   - `uv run clang-format --dry-run --Werror examples/*/*.cpp`
   - `uv run mdformat --check README.md todo.md data/features .github/pull_request_template.md`

Finish with a compact table: step, pass/fail, and the one-line error for failures. If `$ARGUMENTS` contains `clang`, also run steps 3–4 with `--compiler clang++`.
