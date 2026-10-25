# ModernCppAtlas

> 从 C++11 到 C++26 的现代 C++ 特性图谱：**使用场景、真实项目证据、进阶用法、开源库联动**。

[![Content License](https://img.shields.io/badge/content-CC%20BY%204.0-lightgrey)](LICENSE-CONTENT) [![Code License](https://img.shields.io/badge/code-MIT-blue)](LICENSE)

**在线阅读：** <https://zyuhao7.github.io/ModernCppAtlas/>

## 这是什么

不是语法速查表，也不是 API 参考手册。每个特性回答四个问题：

1. 它解决什么问题，没有它时怎么写？
2. 真实开源项目怎么用它（可点击到具体 commit 与行号）？
3. 进阶怎么用，有哪些陷阱和边界？
4. 能和哪些开源库组合，组合的收益与代价是什么？

覆盖范围：C++11、C++14、C++17、C++20、C++23、C++26。

## 不是什么

- 不是 C++ 入门教程（假定读者会 C++03）。
- 不是 cppreference 的全量 API 索引。
- 不做性能排名，只引用可复现的公开数据。
- 不收录无真实项目证据的“纸上特性”。
- 不自己维护编译器支持矩阵，直接引用 cppreference 并注明读取日期。
- 不以条目数量作为成功标准。

## 内容分级

| 级别             | 定位                       | 证据                         | 示例            | 单条耗时  |
| ---------------- | -------------------------- | ---------------------------- | --------------- | --------- |
| **A · 深度条目** | 高频、易误用、有库联动价值 | ≥3 条，至少 1 条来自生产路径 | ≥1 个可编译示例 | 6–10 人时 |
| **B · 标准条目** | 常规特性，讲清用法与边界   | ≥2 条                        | 可选            | 3–5 人时  |
| **C · 索引条目** | 低频或证据稀薄             | ≥1 条                        | 否              | 1–2 人时  |

只有 A/B 级算“完整条目”。C 级页面显示“索引条目，正文待补”，不计入覆盖率与证据强度指标。

## 内容结构

每个特性一个 Markdown 文件，YAML frontmatter + 固定章节。A 级七节：

| 章节         | 内容                                     | 字数上限     |
| ------------ | ---------------------------------------- | ------------ |
| 解决什么问题 | 痛点与旧写法对比                         | 300          |
| 基础用法     | 最小可编译示例 + 输出                    | 1 示例 + 200 |
| 进阶用法     | 组合、泛型、性能相关细节                 | 800          |
| 真实项目证据 | 项目 / 路径 / commit / 用法 / 为什么典型 | 表格         |
| 陷阱与边界   | 生命周期、未定义行为、可移植性           | 500          |
| 库联动       | 与哪些库组合、收益与代价                 | 400          |
| 横向对比     | 替代方案与跨标准演进                     | 500          |

B 级只写问题、基础用法、证据、陷阱、横向对比。写作规则：删除一切不能帮助读者判断“该不该用、怎么用、什么时候别用”的内容。

## 证据标准

一条证据必须同时满足：

- 指向固定 commit，禁止 `main`、`master`、tag 等浮动链接。
- 文件路径与行号可验证，链接格式为 `blob/<commit>/<path>#Lx-Ly`。
- 许可证为 MIT、Apache-2.0、BSD-2/3-Clause、BSL-1.0、CC0-1.0 等宽松协议；MPL-2.0、GPL、AGPL 项目只记录位置，不摘录代码。
- 摘录不超过 15 行、1200 字符，且保留理解用法所需的上下文。
- 排除生成代码、vendored 第三方目录与明显反模式；测试代码可用，但不能作为唯一证据。
- 至少一名审核者确认“用法典型且正确”；`verified` 需要两名不同审核者，其中至少一名不是作者。审核者必须是真实参与复核的人，禁止虚构或挂名。

**生成代码默认拒绝。** 扫描器会跳过 `single_include`、`bundled`、`vendored` 等已知目录，但目录排除只是第一道过滤，不是完整检测。生成文件要作为证据，必须同时满足：

- 它是用户实际使用或分发的唯一形式，手写源不可直接引用；
- 固定 commit 下的内容是稳定的，并且确实展示了预期 API 或惯用法；
- 证据显式标记 `"generated": true`，填写 `submitted_by`，并提供 `generated_review`：至少一名不是提交者的审核人、审核日期与书面理由。

例外会在证据页显示“生成代码（例外已评审）”，不会混在普通证据里。缺少标记、缺少独立审核人或理由为空时 CI 拒绝。

**主张与证据账本。** 特性页 frontmatter 的 `claims` 把可检验的主张映射到证据 ID：

```yaml
claims:
  - id: c1
    statement: "读路径用 shared_lock 快速命中，未命中再升级为 unique_lock 创建并二次检查"
    evidence: [ev-mold-shared-mutex-output-section-001]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
```

规则：

- A 级条目必须提供账本；B、C 级可选，但有账本时同样适用以下约束。
- 一条主张只能引用属于该特性的已发布证据（`manual` / `verified`）。
- 该特性的每条已发布证据都必须被至少一条主张引用；孤证说明页面没有说明它支持什么。
- 账本中的主张必须能在正文或证据页找到对应论述，不能只写 `id` 占位。
- `review.level: manual` 表示至少一人复核，允许作者自审；`verified` 表示两名不同审核者，且至少一人不是作者。`review.reviewers` 必须真实。

账本会渲染到站点页面的“主张与证据”表格，并随正文一起审阅。只有 `verified` 才显示“双审”；单审页面不会伪装成双审。

证据分四级：

| 等级        | 含义                                              | 展示                       |
| ----------- | ------------------------------------------------- | -------------------------- |
| `verified`  | 自动校验 + 两名不同审核者，其中至少一名不是提交者 | 进入正文，绿色标记         |
| `manual`    | 作者人工提交，自动校验通过，尚无第二审核者        | 进入正文，黄色“单审”标记   |
| `candidate` | 扫描器发现但未复核                                | 只进内部报告               |
| `stale`     | 曾可用，但仓库删除、历史改写或许可证变化导致失效  | 从正文移除，保留记录与原因 |

**单审与双审。** `manual` 证据至少一位审核者并记录 `verified_at`；`verified` 证据需要两位不同审核者、至少一位不是 `submitted_by` 填写的提交者，并记录日期。没有第二人真实复核时保持 `manual`。

**关于失效：** 固定 commit 的 permalink 中行号不会漂移。月度扫描的目的不是校验旧证据，而是发现同一特性的更新用法；旧证据只在仓库删除、历史被改写、许可证变化等结构性事件发生时才降级。

## 社区投稿与审核流程

接受社区投稿。流程如下：

1. **提交 Issue / PR**，填写特性 ID、项目 URL、固定 commit、路径与行号、许可证、用法说明、为什么典型。
2. **自动校验（CI）**：
   - commit 与路径存在，行段在文件范围内；
   - permalink 使用固定 commit；
   - 许可证在白名单内；
   - 摘录长度未超限；
   - 证据未重复；
   - 主张账本引用的证据存在且属于该特性，没有未被引用的已发布证据；
   - `review` 与证据审核人数满足单审/双审规则；
   - 生成代码已标记，且例外有独立审核人与书面理由；
   - 示例代码在对应标准下编译通过。
3. **人工复核**：
   - 领域审核：用法是否正确、是否典型、是否反模式；
   - 证据审核：可追溯性、许可证、上下文是否断章取义；
   - 主张审核：正文中可检验的说法是否逐条落入账本，账本是否真的由证据支持。
4. **定级与合并**：通过后按审核人数标记 `manual` 或 `verified`；有争议时以标准文本、提案或编译器实际行为为准，要求提供最小可复现示例。
5. **失效处理**：结构性失效时降级为 `stale` 并发 issue，保留定位记录；月度扫描只生成“更新用法”候选，不自动替换旧证据。

单人维护阶段，新条目默认 `manual`。引入第二名审核者后再开放 `verified`，避免把“单人确认”包装成“双人复核”。

## C++26 收录政策

已投票通过的提案与仍在讨论的草案都收录，但严格标记：

- `status: final`：已进入标准草案（Committee Draft）或已投票通过。
- `status: draft`：仍在演进，页面顶部显示“草案，可能变更”，并注明提案编号与编译器实验性支持开关。
- `status: deprecated`：被标记废弃，给出替代方案与迁移方式。
- `status: removed`：被后续提案替代或移除，保留条目并指向替代方案。

## 本地验证

```bash
uv run tools/fetch_corpus.py                  # 拉取固定 commit 的语料
uv run tools/scan/scan.py                     # 输出候选到 .cache/candidates.json
uv run tools/validate.py --check-corpus       # 校验 schema、固定链接、路径与行段，并比对摘录原文
uv run tools/verify_examples.py --compiler g++ --run  # 编译运行并比对 examples/expected 输出快照
uv run python -m unittest discover -s tests -v  # 工具与输出目录安全边界回归测试
uv run tools/build_site.py && uv run --extra docs mkdocs build --strict  # 生成并构建站点
uv run tools/check_search.py                  # 中文搜索冒烟（需先构建站点）
```

`tools/validate.py` 不带 `--check-corpus` 时只校验数据契约；带该参数时要求 `.cache/corpus` 已就绪，并逐条验证 permalink 指向的 commit、路径、行段、许可证与摘录原文。证据按自身固定 commit 校验，不要求等于语料当前版本；缺失的历史 commit 会自动尝试浅拉取。

示例行为变化时，用 `uv run tools/verify_examples.py --run --update-expected` 更新 `examples/expected/` 下的输出快照，并在 PR 中说明预期变化。

格式化（与 CI 的 `format` job 一致）：

```bash
uv sync --group dev
uv run ruff format tools && uv run ruff check tools
uv run clang-format -i examples/*/*.cpp
uv run mdformat README.md todo.md data/features .github/pull_request_template.md
```

## 参与贡献

- 发现错误或有真实项目用例：提交 Issue。
- 新增或修订条目：提交 PR，并附固定 commit 的证据。
- 讨论标准细节：引用提案编号（Pxxxx 或 Nxxxx）或标准章节。
- 新增特性：先确认它是否有至少一条高质量真实用法；没有则只建 C 级索引条目。

## 许可证

- 正文与结构化数据：CC BY 4.0，全文见 [LICENSE-CONTENT](LICENSE-CONTENT)。
- 示例代码与工具脚本：MIT，全文见 [LICENSE](LICENSE)。
- 第三方项目摘录：遵循原项目许可证，并在证据条目中标注来源与许可证。弱 copyleft（如 MPL-2.0）与 GPL/AGPL 项目只记录位置，不摘录代码。
