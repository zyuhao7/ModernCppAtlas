# Repository Guidelines

## Project Structure

ModernCppAtlas publishes a Chinese-language reference for modern C++. Feature pages live in `data/features/`, evidence records in `data/evidence/`, and library cards in `data/libraries/`. `schema/` defines the data contracts, `tools/` holds Python tooling, and `tests/` holds its tests. Standalone examples live in `examples/cppNN/` with matching `.out` snapshots in `examples/expected/cppNN/`. Regenerate `.site-docs/` and `site/` from source.

## Build and Development

Use Python 3.12 or newer. Run `uv sync --group dev` to install tooling and formatters.

- `uv run tools/validate.py` checks data contracts. Run `uv run tools/fetch_corpus.py` then `uv run tools/validate.py --check-corpus` to verify evidence against pinned source.
- `make examples CXX=g++-14` builds standalone examples under `build/examples/`. Run `make run CXX=g++-14` to execute them.
- `uv run tools/build_site.py && uv run --extra docs mkdocs build --strict` generates and builds the site. After generation, `uv run --extra docs mkdocs serve` previews it locally.

## Style and Naming

Use four spaces for Python and run `uv run ruff format tools tests` and `uv run ruff check tools tests`. Use two spaces for C++ and the Google-based clang-format configuration with its 80-column limit. Format Markdown with mdformat. Match the feature ID to its filename stem and standard prefix, as in `cpp20.erase-if.md`. Preserve required Chinese headings and name example files in `snake_case`.

## Feature Evidence

Pin evidence to a full commit and exact line range. Record the license and keep permitted excerpts within 15 lines and 1,200 characters. Include at least one non-test use in published evidence. Tier A requires at least three published records and a runnable example. Map testable statements to evidence in the `claims` ledger and reference every published record for the feature. On tier A and B pages, place `<!-- compile -->` immediately before the first `cpp` fence under `基础用法`.

## Testing

Run `uv run python -m unittest discover -s tests -v`. Use `test_*.py` modules and `test_*` methods. Run `uv run tools/check_snippets.py --compiler g++-14` for article snippets and `uv run tools/verify_examples.py --compiler g++-14 --run` for deterministic output snapshots. CI also runs standalone examples with Clang 18. After building the site, run `uv run tools/check_links.py` and `uv run tools/check_search.py`.

## Commits and Pull Requests

Use short imperative subjects such as `feat: add erase_if entry`. Use `docs:` for documentation and `fix:` for fixes. Summarize what changed and why. Report checks run and explain intentional snapshot changes. Complete the [pull request template](.github/pull_request_template.md).
