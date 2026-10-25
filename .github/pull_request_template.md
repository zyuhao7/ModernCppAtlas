## 变更内容

<!-- 一句话说明这个 PR 改了什么，以及为什么改。 -->

## 主张与证据账本

- [ ] 新增或修改的可检验主张都已写入特性 frontmatter 的 `claims`。
- [ ] 每条主张引用的证据都属于该特性，并且已发布（`manual` / `verified`）。
- [ ] 该特性的每条已发布证据都至少被一条主张引用，没有孤证。
- [ ] 正文没有账本未覆盖的强断言（版本、行为、性能、编译器支持等）。

## 证据

- [ ] 证据指向固定 commit，路径与行号可在该 commit 下打开。
- [ ] 摘录与固定 commit 原文一致，未超过 15 行 / 1200 字符。
- [ ] 许可证与仓库声明一致；不允许摘录的许可证只记录位置。
- [ ] 证据支持的是主张本身，而不只是“项目里出现过这个 API”。
- [ ] 证据不是生成代码；若是例外，已标记 `"generated": true`，填写 `submitted_by`，并提供非提交者审核的 `generated_review` 与书面理由。

## 审阅

- [ ] `review.reviewers` 与 `verified_by` 都是真实参与复核的人，没有挂名。
- [ ] `manual` 至少一位审核者；`verified` 为两位不同审核者，且至少一位不是作者 / `submitted_by`。
- [ ] 没有第二人复核时，保持 `manual`，没有升级为 `verified`。

## 示例

- [ ] 示例代码可独立编译运行，不是从证据仓库整段复制。
- [ ] 输出与 `examples/expected/` 快照一致；行为变化已在 PR 中说明。
- [ ] 示例展示的是正文主张，而不是无关的 API 演示。

## 自检

- [ ] `uv run tools/validate.py --check-corpus`
- [ ] `uv run python -m unittest discover -s tests -v`
- [ ] `uv run tools/verify_examples.py --run`
- [ ] `uv run tools/build_site.py && uv run --extra docs mkdocs build --strict`
