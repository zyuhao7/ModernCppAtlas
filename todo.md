# 现代 C++ 特性图谱（Modern C++ Atlas）

> 设计文档与实施计划 · v0.1 · 2026-10-24

## 1. 一句话定位

不是又一份语法速查表，而是**以真实项目证据为核心、面向使用场景的 C++11–C++26 特性图谱**：每个特性回答“解决什么问题、真实项目怎么用、进阶怎么用、配合哪些库、什么时候别用”。

## 2. 现状与竞品

| 项目 | 覆盖 | 有场景 | 有真实项目证据 | 有库联动 | C++26 |
|---|---|---|---|---|---|
| [modern-cpp-features](https://github.com/AnthonyCalandra/modern-cpp-features) | C++11–23 | 部分 | 否 | 否 | 否 |
| cppreference | C++98–26 | 否 | 否 | 否 | 部分 |
| Modern C++ Programming Cookbook | C++11–23 | 是（配方） | 否 | 部分 | 否 |
| Awesome C++ / awesome-hpp | 库清单 | 否 | 否 | 是（无语法关联） | 否 |
| C++ Core Guidelines | 规范 | 是 | 少量 | 部分 | 部分 |
| 各类博客/公众号 | 碎片 | 是 | 少量 | 否 | 碎片 |
| **本项目** | **C++11–26** | **是** | **是（可追溯）** | **是** | **是** |

结论：单点内容都有人做，**“特性 × 场景 × 真实项目 × 库联动”的统一索引没人做**，这是唯一值得投入的差异化。若只做前三列，项目没有存在价值。

## 3. 目标与非目标

### 目标

1. 覆盖 C++11 至今每个语言特性与标准库设施，按标准版本、场景、难度建立索引。
2. 每个特性至少 2 条来自知名开源项目的真实用法，固定到 commit + 文件 + 行号，可点击验证。
3. 每个特性给出基础用法、进阶用法、常见陷阱、替代方案、编译器支持矩阵。
4. 给出与主流开源库的组合模式（Abseil、Boost、fmt、TBB、Asio、Drogon、gRPC、protobuf、Eigen、gtest 等）。
5. 所有示例可编译，纳入 CI，防止随标准和编译器演进而腐化。

### 非目标

- 不做 C++ 基础语法教学（假定读者会 C++03）。
- 不做 cppreference 式全量 API 参考。
- 不做性能基准排名（引用现成数据，不自己造）。
- 不复制第三方仓库大段源码，只存定位信息与短引用。

## 4. 内容模型

### 4.1 特性条目

每个特性一个 Markdown 文件，YAML frontmatter + 固定章节：

```yaml
---
id: cpp17.structured-bindings
title: 结构化绑定
standard: C++17
kind: language            # language | library | both
category: [语法糖, 解包]
difficulty: 2             # 1–5
status: active            # active | deprecated | removed
compilers: { gcc: "7", clang: "4", msvc: "19.11" }
tags: [tuple, map, 可读性]
alternatives: [std::tie, .first/.second]
---
```

正文固定七节：

1. **解决什么问题** —— 没有它时的写法与痛点。
2. **基础用法** —— 最小可编译示例 + 输出。
3. **进阶用法** —— 引用成员、数组、自定义类型、`if` 初始化组合、与 `auto&&` 的交互。
4. **真实项目证据** —— 表格：项目 / 路径 / commit / 用法说明 / 为什么这样用。
5. **陷阱与边界** —— 生命周期、结构化绑定无法直接用于 lambda 捕获（C++20 前）、ODR 等。
6. **库联动** —— 与哪些库/设施组合，组合后的收益与代价。
7. **横向对比** —— 替代方案、其他标准版本的演进（如 C++26 的变化）。

### 4.2 真实项目证据条目

```yaml
repo: fmtlib/fmt
commit: <40-hex>
path: include/fmt/format.h
lines: 1234-1240
feature: cpp17.if-constexpr
pattern: 编译期分支选择格式化路径
verified_by: clang-ast   # clang-ast | manual
license: MIT
```

规则：

- 只引用 Apache-2.0 / MIT / BSD 等宽松许可证项目的**短片段**，并标注出处与许可证；GPL 项目只记录位置与文字说明，不贴代码。
- commit 必须固定，禁止 `main` 浮动链接。
- 每条证据经扫描器自动发现后**人工复核**，剔除偶然、反模式、生成代码。
- 每季度重扫，记录失效/漂移。

### 4.3 库联动条目

独立成卡片，可被多个特性引用：

```yaml
library: microsoft/asio
feature: cpp20.coroutines
pattern: co_await + awaitable 实现异步链路
benefit: 消除回调地狱，保持同步式控制流
cost: 协程帧内存、调试栈不直观、需注意生命周期
example: examples/cpp20/coroutine_asio.cpp
```

首批库：Abseil、Boost、fmt、spdlog、nlohmann/json、TBB、Asio、Drogon、gRPC、protobuf、Eigen、GoogleTest、range-v3、tl::expected。

## 5. 技术方案

### 5.1 仓库结构

```text
ModernCppAtlas/
├── README.md                # 本文件
├── data/
│   ├── features/            # 每特性一个 .md（内容真源）
│   ├── evidence/            # 扫描产物 JSON
│   ├── libraries/           # 库联动卡片
│   └── corpus.yaml          # 被扫描仓库清单（含固定 commit）
├── tools/
│   ├── scan/                # clang AST 扫描器
│   ├── verify/              # 示例编译 + 证据链接校验
│   └── build_index.py       # 生成站点索引
├── examples/                # 可编译示例，按特性组织
├── site/                    # 生成产物，不入库
└── .github/workflows/ci.yml
```

### 5.2 证据采集

- **语法特性**：`clang-query` / libclang AST Matcher 按特性建 matcher。例：
  - 结构化绑定 → `decompositionDecl`
  - 协程 → `coroutineBodyStmt`, `coawaitExpr`
  - concepts → `conceptDecl`, `requiresExpr`
  - modules → `moduleDecl`
  - `if constexpr` → `ifStmt` 且含 `constexpr` 条件
- **库特性**：clangd 符号索引 + 限定名匹配（`std::expected`、`std::ranges::`），再用 AST 确认上下文，避免宏/字符串误报。
- **语料库**：20–30 个标杆仓库，固定 commit；优先体量大、风格现代、许可证宽松：LLVM、Abseil、folly、fmt、spdlog、nlohmann/json、DuckDB、ClickHouse、Drogon、gRPC、protobuf、Eigen、cpp-httplib、Taskflow。
- **排序**：项目影响力 × 用法典型性 × 时间新近度；每特性取 3–5 条，人工定稿。
- **增量**：每月扫描，输出新增/消失证据 diff，人工合并。

### 5.3 站点

- MkDocs Material，静态托管 GitHub Pages。
- 三个入口：按标准版本、按使用场景、按难度/替代方案。
- 每页显示编译器支持矩阵与“最后验证时间”。
- 中文优先，英文索引后补。

### 5.4 CI 质量门

1. 所有 `examples/**/*.cpp` 在 GCC 与 Clang 对应标准下编译通过；MSVC 用 windows-latest 跑核心子集。
2. 证据链接校验：repo + commit + 路径 + 行号可访问（行号允许 ±5 漂移）。
3. 每特性必须有 ≥2 条证据、≥1 个陷阱、≥1 个替代方案，缺项让构建失败。
4. 链接检查、拼写检查、表格格式检查。

## 6. 实施计划

单人兼职估算；若全职可压缩约一半。每阶段有可交付物与验收标准，未达标不进入下一阶段。

### P0 · 规格冻结（第 1 周）

- 确定 40 个首批特性（C++17/20 中高价值项）。
- 冻结 frontmatter schema、证据格式、目录结构。
- 搭好仓库骨架与最小 CI。
- 验收：schema 评审通过；一个完整样板条目（建议 `std::expected` 或 `structured bindings`）写完。

### P1 · 证据流水线 MVP（第 2–3 周）

- 实现 5 个 AST matcher + 语料库加载 + JSON 输出。
- 接入 5 个仓库（fmt、Abseil、Drogon、DuckDB、nlohmann/json）。
- 站点骨架上线，展示 P0 的 40 个条目。
- 验收：扫描耗时 <30 分钟；误报率人工抽查 <20%；40 个条目全部有真实证据。

### P2 · C++11/14 完整覆盖（第 4–6 周）

- 语言 + 库特性约 90–110 条。
- 建立“已废弃/被替代”标注（如 `std::auto_ptr`、`std::bind` 的现代替代）。
- 验收：覆盖率 100%；每页示例可编译；至少 3 个库联动卡片。

### P3 · C++17/20（第 7–10 周）

- 约 80–100 条，重点 concepts、ranges、coroutines、modules、`<=>`、`std::span`、`std::format`。
- 完成 10 个库联动卡片。
- 验收：证据自动刷新跑通一次；20 个最常用特性有“进阶用法”长文。

### P4 · C++23/26（第 11–14 周）

- C++23 约 30 条；C++26 约 20 条，明确标注“已定稿/实验性/草案”。
- C++26 重点：反射（P2996）、`std::execution`（P2300）、contracts（P2900）、pack indexing（P2662）、`std::simd` 等。
- 验收：每条 C++26 特性注明编译器实验开关与可用性。

### P5 · 发布与维护（第 15–16 周）

- 全站检索、场景导航、难度路径（“一周速通”“面试高频”“重构旧代码”）。
- 发布 v1.0；建立月度扫描任务与季度人工复核。
- 验收：≥300 条目；≥80% 条目有 ≥2 条证据；CI 全绿；外部 3 人试用反馈。

## 7. 风险与对策

| 风险 | 影响 | 对策 |
|---|---|---|
| 范围爆炸，内容永远写不完 | 项目烂尾 | 特性粒度按“可独立讲述的最小单元”切分；先 40 条 MVP 上线再扩张；明确非目标 |
| 标准/编译器演进导致内容过期 | 可信度下降 | 示例进 CI；证据固定 commit；页面显示“最后验证时间”；季度复核 |
| 真实项目证据误报 | 误导读者 | 自动发现 + 人工复核双轨；低置信度不收录 |
| 许可证问题 | 法律风险 | 只存定位与短引用；GPL 只记位置；逐条标注 license |
| 与 cppreference/速查表同质化 | 无人使用 | 强制差异化：证据、陷阱、库联动；没有证据的条目不允许发布 |
| 单人维护成本过高 | 停更 | 扫描/校验全自动；内容只做增量；社区 PR 模板强制填证据 |
| 中文技术写作质量参差 | 可读性差 | 固定七节模板；每节字数上限；发布前自读与外部评审 |

## 8. 成功度量

- 覆盖：目标条目数完成率。
- 证据强度：有 ≥2 条真实项目证据的条目占比（目标 ≥80%）。
- 可信度：CI 示例编译通过率 100%；证据链接有效率 ≥95%。
- 使用：站点月活、搜索命中率、GitHub star（次要指标，不作目标）。
- 维护：单月更新耗时 ≤8 小时。

## 9. 立即可做的下一步

1. 冻结首批 40 个特性清单（建议从 C++17/20 高频且易被误用的开始）。
2. 写一条完整样板：`structured bindings`，含 3 条真实证据与 1 个库联动。
3. 选定站点方案并跑通本地预览。
4. 建立语料库清单，锁定第一批 5 个仓库的 commit。

## 10. 已确定事项

- 项目名：ModernCppAtlas。
- 语言策略：首发中文，英文版后续补齐。
- 社区投稿：接受；证据采用“自动校验 + 双人复核 + 置信度分级”流程（见 README）。
- C++26：已投票通过的提案与草案均收录；草案在元数据标记 `status: draft`，页面顶部显示“草案，可能变更”并注明提案编号与编译器实验支持。
- 待定：内容与示例的许可证（可能分别选择）。
