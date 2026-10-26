# 现代 C++ 特性图谱（Modern C++ Atlas）

> 设计文档与实施计划 · v0.2 · 2026-10-24

## 1. 一句话定位

不是又一份语法速查表，而是**以真实项目证据为核心、面向使用场景的 C++11–C++26 特性图谱**：每个特性回答“解决什么问题、真实项目怎么用、进阶怎么用、配合哪些库、什么时候别用”。

## 2. 核心判断与建设顺序

### 2.1 三个判断

1. **基础用法是廉价品。** cppreference、modern-cpp-features、教材与博客已经覆盖“语法是什么”。重复这部分不产生项目价值。
2. **真实项目证据是唯一难复制的资产。** 一条可用证据同时要求：固定 commit、可验证路径与行号、宽松许可证、人工判断“用法典型且正确”。这四件事都无法靠搬运完成。
3. **范围必须由证据密度决定，而不是由标准版本铺满。** 一个特性如果找不到两条高质量真实用法，就只做索引条目，不硬写长文。

### 2.2 建设顺序

原计划是“先写正文，再补证据”。调整为：

```text
证据流水线 → 证据页上线 → 深度正文 → 按标准版本扩张
```

理由：证据页可以脱离正文独立成立，能最早验证“是否有人愿意点真实链接”。正文可以在证据稳定后逐步补，且不会因范围铺得太大而烂尾。

### 2.3 项目节奏

这是长期项目，按里程碑与交付物管理，不承诺周数。所有工期按“人时”估算，并给出不同投入强度下的区间。任何阶段未达到退出标准，不进入下一阶段。

## 3. 现状与竞品

| 项目                                                                          | 覆盖         | 有场景     | 有真实项目证据   | 有库联动         | C++26  |
| ----------------------------------------------------------------------------- | ------------ | ---------- | ---------------- | ---------------- | ------ |
| [modern-cpp-features](https://github.com/AnthonyCalandra/modern-cpp-features) | C++11–23     | 部分       | 否               | 否               | 否     |
| cppreference                                                                  | C++98–26     | 否         | 否               | 否               | 部分   |
| Modern C++ Programming Cookbook                                               | C++11–23     | 是（配方） | 否               | 部分             | 否     |
| Awesome C++ / awesome-hpp                                                     | 库清单       | 否         | 否               | 是（无语法关联） | 否     |
| C++ Core Guidelines                                                           | 规范         | 是         | 少量             | 部分             | 部分   |
| 各类博客/公众号                                                               | 碎片         | 是         | 少量             | 否               | 碎片   |
| **本项目**                                                                    | **C++11–26** | **是**     | **是（可追溯）** | **是**           | **是** |

结论：单点内容都有人做，**“特性 × 场景 × 真实项目 × 库联动”的统一索引没人做**，这是唯一值得投入的差异化。若只做前三列，项目没有存在价值。

## 4. 目标与非目标

### 4.1 目标

1. 覆盖 C++11 至今的语言特性与标准库设施，按标准版本、场景、难度建立索引。
2. 每个**完整条目**至少 2 条来自知名开源项目的真实用法，固定到 commit + 文件 + 行号，可点击验证。
3. 每个完整条目给出基础用法、进阶用法、常见陷阱、替代方案；深度条目另有库联动与横向对比。
4. 给出与主流开源库的组合模式（Abseil、Boost、fmt、TBB、Asio、Drogon、gRPC、protobuf、Eigen、GoogleTest 等）。
5. 所有示例可编译，纳入 CI；C++26 示例允许在实验性工具链上标记预期失败。
6. v1.0 目标：≥250 条目，其中 ≥150 条为完整条目（A 级或 B 级）；完整条目中 ≥40% 为 A 级（≥3 条证据）。

### 4.2 非目标

- 不做 C++ 基础语法教学（假定读者会 C++03）。
- 不做 cppreference 式全量 API 参考。
- 不做性能基准排名；只引用可复现的公开数据。
- 不自己维护编译器支持矩阵；直接引用 cppreference 的编译器支持表，并记录引用日期。
- 不复制第三方仓库大段源码，只存定位信息与短引用。
- 不以条目数量作为成功标准；索引条目不计入质量指标。

## 5. 内容分级与覆盖策略

### 5.1 三级条目

| 级别         | 定位                       | 固定章节                           | 证据要求                       | 示例            | 单条耗时  |
| ------------ | -------------------------- | ---------------------------------- | ------------------------------ | --------------- | --------- |
| A · 深度条目 | 高频、易误用、有库联动价值 | 七节全                             | ≥3 条，且至少 1 条来自生产路径 | ≥1 个可编译示例 | 6–10 人时 |
| B · 标准条目 | 常规特性，讲清用法与边界   | 五节：问题、基础、证据、陷阱、替代 | ≥2 条                          | 可选            | 3–5 人时  |
| C · 索引条目 | 低频或证据稀薄，仅建立索引 | 摘要 + 证据表                      | ≥1 条                          | 否              | 1–2 人时  |

“完整条目”仅指 A 级与 B 级。C 级页面必须显示“索引条目，正文待补”标记，不计入覆盖率与证据强度指标。

### 5.2 v1.0 数量模型

| 级别           | 数量    | 平均耗时 | 小计                                              |
| -------------- | ------- | -------- | ------------------------------------------------- |
| A              | 60      | 8 人时   | 480 人时                                          |
| B              | 120     | 4 人时   | 480 人时                                          |
| C              | 70      | 1.5 人时 | 105 人时                                          |
| **内容合计**   | **250** | —        | **约 1065 人时**                                  |
| 工具、站点、CI | —       | —        | 150–250 人时                                      |
| **总计**       | —       | —        | **约 1200–1500 人时**（第 10 节按里程碑给出区间） |

投入强度换算（不含突发维护）：

- 每周 8 小时：约 3–3.6 年。
- 每周 15 小时：约 1.6–1.9 年。
- 全职：约 8–10 个月。

这些是估算，不是承诺。范围可随证据质量下调，不可为凑数上调。

## 6. 内容模型

### 6.1 特性条目

每个特性一个 Markdown 文件，YAML frontmatter + 固定章节。文件名与 `id` 一致，例如 `data/features/cpp17.structured-bindings.md`。

```yaml
---
id: cpp17.structured-bindings
title: 结构化绑定
standard: C++17
kind: language            # language | library | both
category: [语法糖, 解包]
difficulty: 2             # 1–5
status: final             # draft | final | deprecated | removed
tier: A                   # A | B | C
compilers: { gcc: "7", clang: "4", msvc: "19.11" }
tags: [tuple, map, 可读性]
alternatives: [std::tie, .first/.second]
proposal: P0217
last_verified: 2026-10-24
---
```

字段约束：

- `id` 全局唯一，格式 `cpp<版本>.<kebab-case>`，发布后不可改名；改名必须保留重定向表。
- `status` 只表达标准生命周期：`draft`（C++26 草案）、`final`、`deprecated`、`removed`。
- `compilers` 为最低可用版本；必须附 cppreference 链接与读取日期。
- `last_verified` 由证据流水线自动写入，不允许手填。
- 证据等级单独存放在证据条目中，不写进特性 frontmatter，避免一个字段承担两种含义。

正文七节与字数上限（A 级）：

| 章节         | 内容                                           | 上限              |
| ------------ | ---------------------------------------------- | ----------------- |
| 解决什么问题 | 痛点、旧写法、代价                             | 300 字            |
| 基础用法     | 最小可编译示例 + 输出                          | 1 个示例 + 200 字 |
| 进阶用法     | 组合、泛型、性能细节                           | 800 字            |
| 真实项目证据 | 表格：项目 / 路径 / commit / 用法 / 为什么典型 | 表格，不设字数    |
| 陷阱与边界   | 生命周期、UB、可移植性、常见误用               | 500 字            |
| 库联动       | 与哪些库组合、收益与代价                       | 400 字            |
| 横向对比     | 替代方案、跨标准演进                           | 500 字            |

写作规则：删除一切不能帮助读者判断“该不该用、怎么用、什么时候别用”的内容。禁止复制 cppreference 段落，禁止无出处的性能断言。

B 级只写“解决什么问题、基础用法、真实项目证据、陷阱与边界、横向对比”，其余章节省略。

### 6.2 证据条目

```yaml
id: ev-fmt-if-constexpr-001
repo: fmtlib/fmt
commit: <40-hex>
path: include/fmt/format.h
lines: 1234-1240
feature: cpp17.if-constexpr
pattern: 编译期选择格式化路径
why_typical: 该分支覆盖整条格式化主路径，不是边缘测试用例
excerpt: |
  ...
license: MIT
level: verified            # verified | manual | candidate | stale
discovered_by: tree-sitter
verified_by: [alice, bob]
verified_at: 2026-10-24
notes: ""
```

规则：

- 链接必须是固定 commit 的 permalink：`https://github.com/{owner}/{repo}/blob/{commit}/{path}#L{start}-L{end}`。
- 摘录不超过 15 行、不超过 1200 字符；只保留理解用法所需的上下文。
- 许可证必须在固定 commit 的 `LICENSE` 文件上核对，并在条目中记录许可证标识与核对日期。
- 许可证白名单：MIT、Apache-2.0、BSD-2/3-Clause、BSL-1.0、CC0-1.0。
- MPL-2.0、GPL、AGPL 等项目只记录定位与文字说明，不摘录代码。
- 排除生成代码、vendored 第三方目录、明显反模式；测试代码可用，但必须在 `notes` 标明，且不能作为唯一证据。
- 同一 repo + commit + path + 行段 + feature 只允许一条证据，防止重复计数。
- 一条证据只描述一种用法模式，不把多个特性混在一条记录里。

### 6.3 库联动卡片

```yaml
id: lib-asio-coroutines
library: chriskohlhoff/asio
feature: cpp20.coroutines
pattern: co_await + awaitable 实现异步链路
benefit: 消除回调地狱，保持同步式控制流
cost: 协程帧内存、调试栈不直观、对象生命周期要求严格
evidence: [ev-asio-coawait-001, ev-asio-awaitable-002]
example: examples/cpp20/coroutine_asio.cpp
```

首批库：Abseil、Boost、fmt、spdlog、nlohmann/json、TBB、Asio、Drogon、gRPC、protobuf、Eigen、GoogleTest、range-v3、tl::expected。

### 6.4 Schema 即代码

`feature`、`evidence`、`library` 三类数据各自有 JSON Schema，放在 `schema/`。CI 必须用 schema 校验，不允许“文档写一套、数据长另一套”。schema 变更视为破坏性变更，需要迁移脚本与版本号。

## 7. 证据流水线

### 7.1 流水线阶段

| 阶段       | 输入           | 输出                                | 人工参与          |
| ---------- | -------------- | ----------------------------------- | ----------------- |
| 语料入库   | `corpus.yaml`  | 固定 commit 的稀疏克隆、许可证快照  | 选定仓库与 commit |
| 语法扫描   | 源码           | 候选位置（文件、行段、信号）        | 无                |
| 过滤与排序 | 候选位置       | 去重、排除生成/vendor、按影响力排序 | 抽查误报          |
| 人工复核   | 排好序的候选   | 证据 JSON、`why_typical`、许可证    | 全部              |
| 发布       | 证据 JSON      | 站点卡片、permalink                 | 无                |
| 增量刷新   | 新 commit 语料 | 新增/消失候选 diff                  | 决定是否替换      |

### 7.2 先便宜后昂贵

第一版扫描器只做**语法级信号**，用 Tree-sitter 查询，不要求项目可编译。典型查询点（以锁定的 tree-sitter-cpp 版本为准）：

| 特性           | 语法信号                                                            |
| -------------- | ------------------------------------------------------------------- |
| 结构化绑定     | `structured_binding_declarator`                                     |
| `if constexpr` | `if_statement` 且含 `constexpr`                                     |
| 协程           | `co_await_expression`、`co_yield_expression`、`co_return_statement` |
| concepts       | `concept_definition`、`requires_clause`、`requires_expression`      |
| modules        | `module_declaration`、`import_declaration`                          |
| lambda         | `lambda_expression`，再按捕获与参数细分                             |
| 标准库设施     | `#include <optional>` 等头文件 + 限定名文本匹配（`std::optional`）  |

只有语义类特性需要 AST 二次确认，例如：concepts 的约束实际生效位置、`consteval` 的求值上下文、重载决议后的调用目标。二次确认使用**局部编译单元**：从候选位置抽取最小片段，或用仓库自带的单目录 compile commands，不构建整个仓库。

明确不做：

- 不为 LLVM、ClickHouse、DuckDB 这类巨型仓库生成全量 `compile_commands.json`。
- 不把“整库构建成功”作为扫描前提。
- 不追求一开始就零误报。候选队列允许误报，人工复核负责剔除。

### 7.3 语料库

首批 4 个（M0）：fmt、nlohmann/json、spdlog、mold。共同点：体量可控、MIT、现代 C++ 用法密集。mold 是 C++20 项目，用来补齐 `std::span`、字符串前缀/后缀检查的生产路径证据。

M1 扩到 7 个：增加 Abseil、Drogon、DuckDB（2026-10-25 已完成，4,489 个源文件、2,520 条候选）。

**M0 首轮扫描（2026-10-24，fmt 12.2.0 / json 3.12.0 / spdlog 1.17.0 / mold 2.35.0）**：加入测试目录与 mold 后重扫，505 个源文件、1351 条候选；已排除 `single_include`、`bundled` 等生成或 vendored 目录。下表是扫描器候选数，不是已复核证据数。

| 特性                   | 生产候选 | 测试候选 | M0 决策                             |
| ---------------------- | -------: | -------: | ----------------------------------- |
| `if constexpr`         |      144 |        0 | A 级样板已完成                      |
| `std::filesystem`      |       24 |       10 | A                                   |
| 字符串/范围小工具      |      110 |       22 | A                                   |
| `<=>`                  |        7 |        3 | A                                   |
| concepts               |       60 |        0 | A                                   |
| `std::byte`            |        4 |        3 | A                                   |
| `std::optional`        |       56 |       34 | A                                   |
| `std::span`            |      157 |        1 | A                                   |
| `std::string_view`     |      366 |      251 | A                                   |
| ranges                 |        1 |       18 | A（生产证据仅 1 条，另用测试补充）  |
| `std::bit_cast`        |        2 |        1 | 不足 3 条，B 或扩语料               |
| `std::format`          |        2 |        0 | 不足 3 条，B 或扩语料               |
| `std::source_location` |        2 |        1 | 不足 3 条，B 或扩语料               |
| structured bindings    |        9 |       18 | A（3 条生产证据，已作为全流程样板） |
| `std::variant`         |        6 |       16 | 不足 3 条生产证据，B 或扩语料       |
| `std::any`             |        1 |        4 | 不足 3 条，B 或扩语料               |
| 折叠表达式             |        1 |        0 | 不足 3 条，B 或扩语料               |

结论：M0 的 10 个 A 级从 `if constexpr`、`std::filesystem`、字符串/范围小工具、`<=>`、concepts、`std::byte`、`std::optional`、`std::span`、`std::string_view`、ranges 中选取。`structured bindings` 在第二轮复核中找到 3 条生产证据（mold 2 条、spdlog 1 条），作为新增特性的全流程样板；在 M0 阶段，`std::variant`、CTAD 等仍留在候选中，不为凑 A 级而放宽标准。

**当前状态（2026-10-26 复核）**：上表保留 M0 扫描时的初始判断；后续人工复核已依据生产证据把 `std::any`、`std::variant`、`std::bit_cast`、`std::format`、`std::source_location` 等升为 A 级。当前站点共 29 个完整条目，其中 28 个 A、1 个 B（`cpp20.requires-expressions`）。

M1 扩充语料后，折叠表达式有 7 条候选（Abseil、fmt、Drogon 生产路径各选 1 条），协程有 125 条候选（Drogon 3 条生产路径）；两者已完成 A 级条目。

后续候选与许可证（以仓库固定 commit 的 LICENSE 为准）：

| 仓库                     | 许可证                         | 备注                           |
| ------------------------ | ------------------------------ | ------------------------------ |
| fmtlib/fmt               | MIT                            | 首批                           |
| nlohmann/json            | MIT                            | 首批                           |
| gabime/spdlog            | MIT                            | 首批                           |
| rui314/mold              | MIT                            | 首批；补齐 span 与字符串小工具 |
| abseil/abseil-cpp        | Apache-2.0                     | M1                             |
| drogonframework/drogon   | MIT                            | M1                             |
| duckdb/duckdb            | MIT                            | M1                             |
| llvm/llvm-project        | Apache-2.0 WITH LLVM-exception | 只扫选定子目录                 |
| ClickHouse/ClickHouse    | Apache-2.0                     | 只扫选定子目录                 |
| protocolbuffers/protobuf | BSD-3-Clause                   | 后续                           |
| grpc/grpc                | Apache-2.0                     | 后续                           |
| oneapi-src/oneTBB        | Apache-2.0                     | 后续                           |
| chriskohlhoff/asio       | BSL-1.0                        | 后续                           |
| taskflow/taskflow        | MIT                            | 后续                           |
| yhirose/cpp-httplib      | MIT                            | 后续                           |
| ericniebler/range-v3     | BSL-1.0                        | 后续                           |
| google/googletest        | BSD-3-Clause                   | 后续                           |
| TartanLlama/expected     | CC0-1.0                        | 后续                           |
| eigenweb/eigen           | MPL-2.0                        | 只记录位置，不摘录             |

### 7.4 排序与去重

候选排序：项目影响力 × 用法典型性 × 时间新近度。

- 项目影响力：GitHub star 只作粗筛，最终由人工判断。
- 用法典型性：是否覆盖库的主路径、是否代表该特性的常见模式。
- 时间新近度：同等条件下优先较新的 commit，但不为了新而牺牲典型性。

去重信号：`repo + path + 归一化行段 + feature`。同一文件内相邻 20 行内的同类用法视为一条。

### 7.5 证据生命周期（修正旧规则）

固定 commit 的 permalink 中，**行号不会漂移**。因此“每月重扫以检测行号漂移”是伪需求。真正会让证据失效的情况只有：

1. 仓库被删除、私有化或整体迁移；
2. 历史被改写（force-push、仓库重建），固定 commit 不可达；
3. 上游当前许可证变更，导致项目引用政策需要重新评估（固定 commit 的许可证本身不变）；
4. GitHub 平台不可用或链接格式变化。

处理规则：

- 每月任务：扫描语料的新 commit，产出“更新用法”候选，由人工决定是否替换；不自动改旧证据。
- 旧证据仅在上述结构性失效时降级为 `stale`，从正文隐藏，保留数据与失效原因。
- 页面展示 `last_verified` 与“固定 commit”标记，不展示会随时间变化的“当前有效性”。

### 7.6 成本边界

- 稀疏克隆 20 个仓库约数 GB；本地缓存，CI 只拉取所需 commit。
- 语法扫描应在分钟级完成；真正的瓶颈是人工复核，因此候选排序必须尽量准。
- 扫描器的查询集只覆盖已发布的特性，不为未写条目的特性提前建 matcher。

## 8. 站点与搜索

- MkDocs Material，静态托管 GitHub Pages。
- 中文优先；中文搜索依赖分词，构建环境必须安装 `jieba`，并在 CI 中用固定查询集验证搜索命中。
- 三个入口：按标准版本、按使用场景、按难度/替代方案。
- 页面元素：标准版本、难度、`status`、证据等级、`last_verified`、固定 commit 链接、编译器支持引用链接。
- 证据页先行：即使正文未写，也展示“项目 / 路径 / commit / 用法说明 / 许可证”。
- 不引入前端框架与自定义搜索服务；静态站点优先，降低维护成本。

## 9. CI 质量门

| 检查              | 阻断 | 说明                                                                               |
| ----------------- | ---- | ---------------------------------------------------------------------------------- |
| Schema 校验       | 是   | feature / evidence / library 三类数据                                              |
| ID 唯一与交叉引用 | 是   | 特性、证据、库卡片的引用必须存在                                                   |
| 证据必填字段      | 是   | commit、path、lines、license、why_typical                                          |
| permalink 格式    | 是   | 必须是固定 commit，禁止 `main`/`master`/tag 浮动链接                               |
| 路径与行段存在性  | 是   | 用本地语料克隆执行 `git cat-file -e <commit>:<path>`，再核对行数；不逐链接请求网络 |
| 许可证白名单      | 是   | 白名单之外只能记录位置                                                             |
| 摘录长度          | 是   | ≤15 行且 ≤1200 字符                                                                |
| 重复证据          | 是   | 同 repo + commit + path + 行段 + feature                                           |
| 示例编译          | 是   | GCC 与 Clang 对应标准；MSVC 跑核心子集                                             |
| 正文代码块编译    | 是   | A/B 级“基础用法”首个代码块须带 `<!-- compile -->`，按页面标准编译                  |
| 编译器下限一致    | 是   | 各特性 frontmatter 与 `data/compilers.yaml` 一致，来源指向对应 cppreference 表     |
| C++26 示例        | 条件 | 实验性工具链缺失时标记 `xfail`，必须写明原因                                       |
| 章节完整性        | 是   | A/B 级条目缺节直接失败                                                             |
| 字数上限          | 警告 | 超过上限提示删减，不阻断                                                           |
| 内链检查          | 是   | 站内链接必须可达                                                                   |
| 中文搜索冒烟      | 是   | 固定查询集命中率不低于阈值                                                         |

PR CI 与月度刷新任务分离：PR CI 只验证已固定 commit 的数据；月度任务负责发现新用法并生成 diff。

## 10. 实施路线

### M0 · Tracer Bullet（100–160 人时）

- [x] 锁定 4 个语料仓库：fmt、nlohmann/json、spdlog、mold。
- [x] 扫描器 v0：Tree-sitter + 正则候选，输出 `.cache/candidates.json`；加入测试目录与 mold 后重扫为 505 个文件、1351 条候选。
- [x] schema、validator、固定 commit 路径与行段校验。
- [x] 首个 A 级样板：`if constexpr`，3 条证据，GCC C++17 编译通过。
- [x] 其余 9 个 A 级条目，按第 7.3 节扫描结果选取；每条 3 条证据，示例编译通过。
- [x] 站点上线：17 个特性页 + 50 个证据页，`--strict` 构建通过，中文搜索冒烟通过。
- [x] CI：schema、permalink、路径、许可证白名单、示例编译、站点构建与搜索。
- 退出标准：50 条证据全部可点击验证；16 个示例编译通过；3 名外部读者能在 2 分钟内找到“某个特性在哪个项目里怎么用”。

### M1 · 公开 Alpha（新增 250–350 人时）

- 40 个完整条目，其中至少 32 个达到 A 级（≥3 条证据），其余为 B 级。
- 6 个语料仓库（已扩到 7：Abseil、Drogon、DuckDB 已加入）。
- 3 张库联动卡片。
- 证据等级制度上线：初期内容标注 `manual`，引入第二名审核者后才标 `verified`。
- 退出标准：40/40 条目至少 2 条证据，其中 32/40 达到 3 条；permalink 校验 100% 通过；外部 3 人试用并给出书面反馈。

### M2 · C++11/14 覆盖（200–300 人时）

- 累计 100 条目：40 A + 60 B。
- 建立“已废弃/被替代”标注（`std::auto_ptr`、`std::bind` 的现代替代等）。
- 退出标准：C++11/14 高频特性无空缺；所有 B 级条目至少 2 条证据；站点导航按标准版本可用。

### M3 · C++17/20 扩张（新增 350–450 人时）

- 累计 200 条目：60 A + 110 B + 30 C。
- 20 篇进阶长文：concepts、ranges、coroutines、modules、`std::format`、`std::span` 等。
- 12 张库联动卡片。
- 退出标准：月度证据刷新跑通一次；20 个最常用特性有完整进阶用法。

### M4 · C++23/26（新增 150–250 人时）

- 累计 250 条目：60 A + 120 B + 70 C。
- C++23 重点：`std::expected`、`std::mdspan`、`std::print`、`std::flat_map`、`std::stacktrace`、`std::generator`、deducing this。
- C++26 重点：反射（P2996）、`std::execution`（P2300）、contracts（P2900）、pack indexing（P2662）、`std::simd`。
- 每条 C++26 特性注明提案编号、`status`、编译器实验开关。
- 退出标准：C++26 内容全部带状态标记；不存在把草案写成定稿的条目。

### M5 · v1.0 与维护

- 质量目标：≥150 完整条目；其中 A 级占比 ≥40%；permalink 有效率 ≥95%；示例编译通过率 100%。
- 维护目标：月度扫描 + 季度人工复核，总计 ≤8 小时/月。
- 发布后建立外部审核者名单；`verified` 必须由非作者审核者确认。

## 11. 首批 40 个特性（M1 Alpha）

选择标准：C++17/20 高频、易误用、在首批语料库中证据密度高。目标为 A 级；证据不足 3 条时降为 B 级。

| #   | 特性                             | ID                                 | 首批重点                                              |
| --- | -------------------------------- | ---------------------------------- | ----------------------------------------------------- |
| 1   | 结构化绑定                       | `cpp17.structured-bindings`        | lambda 捕获限制、隐藏拷贝                             |
| 2   | `if constexpr`                   | `cpp17.if-constexpr`               | 丢弃分支仍需语法合法                                  |
| 3   | 内联变量                         | `cpp17.inline-variables`           | 头文件 ODR、与 `static` 的区别                        |
| 4   | CTAD                             | `cpp17.ctad`                       | 推导指引、与 `initializer_list` 的冲突                |
| 5   | 折叠表达式                       | `cpp17.fold-expressions`           | 空包结果、运算符优先级                                |
| 6   | constexpr lambda                 | `cpp17.constexpr-lambda`           | 隐式 constexpr 条件、捕获规则                         |
| 7   | if/switch 初始化语句             | `cpp17.init-statement`             | 作用域、与 `if constexpr` 组合                        |
| 8   | 属性 `[[nodiscard]]` 等          | `cpp17.attributes`                 | 位置、实现差异、被忽略的后果                          |
| 9   | 保证的复制消除                   | `cpp17.guaranteed-copy-elision`    | 与 RVO/NRVO 的区别、具名返回值                        |
| 10  | `std::optional`                  | `cpp17.optional`                   | 值语义、`operator*` 不检查                            |
| 11  | `std::variant`                   | `cpp17.variant`                    | `valueless_by_exception`、访问方式                    |
| 12  | `std::string_view`               | `cpp17.string-view`                | 悬垂、非空终止、生命周期                              |
| 13  | `std::filesystem`                | `cpp17.filesystem`                 | 错误码与异常、路径编码                                |
| 14  | `std::any`                       | `cpp17.any`                        | 类型擦除成本、无接口约束                              |
| 15  | `std::invoke` / `std::apply`     | `cpp17.invoke-apply`               | 成员指针、完美转发                                    |
| 16  | `std::scoped_lock`               | `cpp17.scoped-lock`                | 死锁避免、与 `lock_guard` 的区别                      |
| 17  | `std::shared_mutex`              | `cpp17.shared-mutex`               | 读写饥饿、性能边界                                    |
| 18  | 并行算法                         | `cpp17.parallel-algorithms`        | 执行策略、异常、线程开销                              |
| 19  | `std::byte`                      | `cpp17.byte`                       | 不是字符类型、别名规则                                |
| 20  | `std::pmr`                       | `cpp17.pmr`                        | 分配器生命周期、monotonic buffer                      |
| 21  | concepts                         | `cpp20.concepts`                   | 约束顺序、错误信息、与 SFINAE 的取舍                  |
| 22  | requires 表达式                  | `cpp20.requires-expressions`       | 四类要求、未求值上下文                                |
| 23  | 缩写函数模板                     | `cpp20.abbreviated-templates`      | 与 `auto` 参数的区别、重载顺序                        |
| 24  | ranges 与 views                  | `cpp20.ranges`                     | view 悬垂、惰性求值、性能                             |
| 25  | `consteval` / `constinit`        | `cpp20.consteval-constinit`        | 立即函数、静态初始化顺序                              |
| 26  | 协程                             | `cpp20.coroutines`                 | 生命周期、帧分配、调度器                              |
| 27  | modules                          | `cpp20.modules`                    | 构建系统支持、宏隔离                                  |
| 28  | 三路比较 `<=>`                   | `cpp20.spaceship`                  | 自动生成规则、返回类型                                |
| 29  | 指定初始化                       | `cpp20.designated-initializers`    | 顺序、聚合类型限制                                    |
| 30  | 模板 lambda                      | `cpp20.template-lambda`            | 显式模板参数、捕获                                    |
| 31  | `std::span`                      | `cpp20.span`                       | 生命周期、const 语义                                  |
| 32  | `std::format`                    | `cpp20.format`                     | 编译期格式串、编译器支持                              |
| 33  | `std::jthread` / `stop_token`    | `cpp20.jthread-stop-token`         | 协作式取消、析构顺序                                  |
| 34  | `std::source_location`           | `cpp20.source-location`            | 内联、宏包装后的位置                                  |
| 35  | `std::bit_cast`                  | `cpp20.bit-cast`                   | trivially copyable、大小一致                          |
| 36  | `std::atomic_ref`                | `cpp20.atomic-ref`                 | 对齐、对象生命周期                                    |
| 37  | latch / barrier / semaphore      | `cpp20.latch-barrier-semaphore`    | 复用、超时、协作取消                                  |
| 38  | constexpr 动态分配               | `cpp20.constexpr-allocation`       | 瞬态分配规则、C++20 限制                              |
| 39  | `remove_cvref` / `type_identity` | `cpp20.remove-cvref-type-identity` | 元编程组合、转发场景                                  |
| 40  | 字符串/范围小工具                | `cpp20.string-range-utilities`     | `ssize`、`starts_with`、`contains`、`erase_if` 的边界 |

清单是目标，不是承诺。扫描后若某特性在语料库中凑不齐 3 条高质量证据，降为 B/C 级或替换，不为凑数保留。

后续版本补充 C++11/14（智能指针、移动语义、`std::thread`、lambda、`constexpr`、变参模板、`std::array` 等）与 C++23/26。

## 12. 风险与对策

| 风险                           | 影响             | 对策                                                            |
| ------------------------------ | ---------------- | --------------------------------------------------------------- |
| 范围爆炸，内容永远写不完       | 项目烂尾         | 三级条目；先 10 条 tracer bullet；证据不足只做 C 级；不承诺周数 |
| 单人审核导致“verified”名不副实 | 可信度崩塌       | 单人阶段只标 `manual`；`verified` 必须第二审核者；页面明示等级  |
| 扫描器成本失控                 | 工具吞掉内容时间 | 语法扫描优先；AST 只做局部确认；不为巨型仓库建全量编译库        |
| 固定链接失效                   | 少量页面不可用   | 本地语料缓存 + `git cat-file` 校验；结构性失效才降级 `stale`    |
| 第三方许可证问题               | 法律风险         | 白名单；固定 commit 核对 LICENSE；弱 copyleft 只记位置          |
| C++26 标准仍在变化             | 内容过期         | `status` + 提案编号 + 实验开关；草案页面顶部提示                |
| 中文搜索质量差                 | 站点不可用       | MkDocs + jieba；CI 固定查询集冒烟                               |
| 社区冷启动                     | 没人投稿         | tracer bullet 先公开；证据页可独立传播；PR 模板降低门槛         |
| 长期项目动力衰减               | 停更             | 里程碑制；每个阶段有可展示成果；范围可缩不可灌水                |

## 13. 成功度量

按阶段设阈值，不以 star 数为主目标。

| 指标                          | M0   | M1       | v1.0     |
| ----------------------------- | ---- | -------- | -------- |
| A 级条目                      | 10   | ≥32      | 60       |
| 完整条目                      | 10   | 40       | 150      |
| A 级占完整条目比例            | 100% | ≥80%     | ≥40%     |
| permalink 路径/行段校验通过率 | 100% | 100%     | ≥95%     |
| 示例编译通过率                | 100% | 100%     | 100%     |
| 库联动卡片                    | 0    | 3        | 20       |
| 外部试用反馈                  | 3 人 | 3 人书面 | 持续收集 |
| 月维护耗时                    | —    | —        | ≤8 小时  |

## 14. 立即可做的下一步

- [x] 写 `schema/feature.schema.json`、`schema/evidence.schema.json`、`schema/library.schema.json`、`schema/corpus.schema.json`，并用样板条目跑通校验。
- [x] 锁定 fmt、nlohmann/json、spdlog、mold 的 commit，建立 `data/corpus.yaml` 与稀疏克隆脚本。
- [x] 用 Tree-sitter 扫描四个仓库，重扫输出 1351 条候选；密度见第 7.3 节。
- [x] 写首个 A 级样板：`if constexpr`，3 条证据，示例在 GCC C++17 下编译通过。
- [x] 按 7.3 节结果写完其余 9 个 A 级条目，每条 3 条证据。
- [x] 搭 MkDocs 骨架与证据页模板，配置 jieba 中文搜索。
- [x] 上最小 CI：schema、permalink、路径行段、许可证白名单、示例编译。

## 15. 已确定事项

- 项目名：ModernCppAtlas。
- 语言策略：首发中文，英文版后续补齐。
- 建设顺序：证据流水线 → 证据页 → 深度正文。
- 内容分级：A/B/C，完整条目仅计 A/B。
- 审核制度：单人阶段标 `manual`；`verified` 需第二审核者。
- 证据固定 commit；固定 commit 的行号不漂移，只有结构性失效才降级 `stale`。
- C++26：已投票通过的提案与草案均收录；草案标记 `status: draft`，页面提示可能变更。
- 许可证：正文与数据采用 CC BY 4.0；示例代码采用 MIT；第三方摘录遵循原许可证并标注。
- 主张账本：A 级必须提供；每条发布证据必须被至少一条主张引用。
- 生成代码：默认拒绝；例外需 `generated: true`、`submitted_by`、独立审核人与书面理由。
- 正文代码块：只用 `<!-- compile -->` 标记需要编译的围栏；A/B 级“基础用法”首个 C++ 围栏必须标记，其余片段不强制。
- 编译器下限：`data/compilers.yaml` 是唯一来源，frontmatter 必须与之一致并带 cppreference 链接与核对日期。

## 16. 待定事项

- 站点域名与托管方式（GitHub Pages 为默认方案）。
- 外部审核者的邀请与轮值机制。
- 英文版启动时间。
