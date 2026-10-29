---
id: cpp20.modules
title: 模块（modules）
standard: C++20
kind: language
category: [模块]
difficulty: 4
status: final
tier: B
compilers: {gcc: "11", clang: "8", msvc: "19.28"}
tags: [modules, import, export, 构建, 宏隔离]
alternatives: [头文件与 include guard, 预编译头, 头文件单元]
proposal: P1103R3
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "用模块接口单元导出整个格式化库，并按配置选择 import std"
    evidence: [ev-fmt-modules-export-001]
  - id: c2
    statement: "客户端只写 import fmt 就能使用库，并验证命名空间可见"
    evidence: [ev-fmt-modules-import-test-002]
  - id: c3
    statement: "测试确认模块导入不会泄漏实现头文件的宏"
    evidence: [ev-fmt-modules-macro-isolation-003]
---

## 解决什么问题

头文件模型靠文本包含工作：每个翻译单元重新解析整棵包含树，宏会跨头文件泄漏，包含顺序影响语义，同一实体在多个翻译单元中的定义还要靠 ODR 维持一致。预编译头能缓解解析成本，但不改变宏和包含模型。

C++20 模块把接口编译成二进制模块接口，客户端用 `import` 取得声明。编译器不必重新解析实现细节，宏也不会随导入进入客户端。代价是构建系统必须理解模块依赖，并调用编译器的模块扫描流程。

## 基础用法

模块接口单元用 `export module` 声明模块名，需要暴露的声明再加 `export`：

```text
// math.cppm
export module math;

export int add(int a, int b) { return a + b; }

// main.cpp
import math;

int main() { return add(1, 2); }
```

这不是只改源码就能生效的特性：构建时要先生成模块接口，再编译导入方。GCC 需要 `-fmodules-ts`，Clang 需要预编译模块接口并传 `-fmodule-file`，MSVC 需要 `/interface` 与 `/ifcOutput` 等选项。fmt 用 CMake 的 `add_module_library` 把差异封装在一处。

上面的多文件示例无法用当前仓库的单文件编译流程验证；下面只编译特性检测部分：

<!-- compile -->

```cpp
#include <iostream>

int main() {
#ifdef __cpp_modules
  std::cout << "modules: " << __cpp_modules << '\n';
#else
  std::cout << "modules: unavailable\n";
#endif
}
```

特性测试宏存在，不代表构建系统已经接通；实际项目仍要按编译器配置模块编译选项。fmt 的接口单元先写 `module;` 进入全局模块片段，包含标准库与实现头文件，再用 `export module fmt;` 开始模块范围。这样传统头文件中的声明不会被意外归入 `fmt` 模块。

## 进阶用法

模块可以拆成分区：`export module fmt:format;` 定义分区，接口单元用 `export import :format;` 重新导出。实现单元写 `module fmt;` 而不写 `export`，其中的定义只在本模块内可见。

`export { ... }` 块可以批量导出声明；fmt 用 `FMT_EXPORT`、`FMT_BEGIN_EXPORT` 宏在不同构建模式下切换。`FMT_IMPORT_STD` 打开时用 `import std;` 替代标准库头文件，否则仍走传统包含。

模块接口变化后导入方仍需重新编译，模块没有提供跨版本 ABI 稳定性。分区和私有模块片段 `module :private;` 的编译器支持也不一致，跨编译器项目应先用小规模接口验证工具链。

## 真实项目证据

| 项目 | 位置                                                                                                                                     | 用法                                                      | 为什么典型                                                  |
| ---- | ---------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------- | ----------------------------------------------------------- |
| fmt  | [`src/fmt.cc#L101-L105`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/src/fmt.cc#L101-L105)               | 用 `export module fmt;` 导出整个库，并按配置 `import std` | 生产库用单个接口单元聚合公开头文件，客户端只写 `import fmt` |
| fmt  | [`test/module-test.cc#L34-L44`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/test/module-test.cc#L34-L44) | 导入后直接使用 `fmt` 与 `fmt::literals` 命名空间          | 验证模块导入的名字查找与命名空间可见性                      |
| fmt  | [`test/module-test.cc#L46-L51`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/test/module-test.cc#L46-L51) | 断言 `FMT_BASE_H_` 等宏没有被导入                         | 把“宏不随模块导出”从标准条款变成可执行检查                  |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。第一条是生产构建路径，后两条是 fmt 的模块测试。当前以 B 级发布，是因为模块需要多翻译单元与构建系统配合，无法纳入仓库的单文件示例编译流程。

## 陷阱与边界

**工具链支持不完整。** cppreference 将 GCC 11 与 Clang 8 起的模块支持标为部分实现，MSVC 19.28 才列为完整支持；实际可用性还取决于构建系统和编译器选项。

**宏不进入模块接口。** `import` 不会带来被导入头文件的宏。依赖宏配置的库必须在接口中显式提供常量或函数，或要求客户端自行定义宏。

**模块范围内的 `#include` 会附着到模块。** 把传统头文件放在 `export module` 之后包含，其中的声明会被视为模块的一部分；应像 fmt 一样放进全局模块片段。

**`import std;` 不是所有标准库都可用。** 标准库模块化依赖实现；fmt 用 `FMT_IMPORT_STD` 做成可选项，而不是默认要求。

**接口变更仍触发重新编译。** 模块减少重复解析，但不提供稳定的二进制接口；导出声明变化后，导入方必须重建。

**头文件与模块可以共存，但不要在同一翻译单元里混用同一实体。** 一个库同时以头文件和模块形式发布时，客户端应选择一种包含方式，避免重复定义。

**构建系统必须扫描依赖。** 模块顺序不能靠文件系统顺序推断；CMake、Ninja 等需要 C++20 模块支持，旧构建脚本通常要改造。

**分区与私有模块片段的实现差异大。** 先用目标编译器验证，再决定是否把公共接口拆成分区。

## 库联动

- **fmt**：`src/fmt.cc` 是生产用模块接口单元，CMake 的 `add_module_library` 负责 GCC、Clang、MSVC 的模块编译差异。
- **CMake**：`CXX_MODULES` 文件集或手工 `-fmodule-file` 传播模块接口；构建系统的支持程度直接决定模块能否落地。
- **与头文件单元的分工**：头文件单元把现有头文件包装成可导入单元，适合渐进迁移；命名模块需要库作者提供接口单元。

## 横向对比

| 方案       | 宏隔离 | 重复解析       | 构建改造 | 主要代价                             |
| ---------- | ------ | -------------- | -------- | ------------------------------------ |
| 头文件     | 否     | 每个翻译单元   | 无       | 包含顺序、宏污染、解析成本           |
| 预编译头   | 否     | 部分避免       | 中等     | 与编译选项绑定，宏状态仍需一致       |
| 头文件单元 | 是     | 部分避免       | 中等     | 依赖编译器对头文件单元的支持         |
| 命名模块   | 是     | 接口只解析一次 | 高       | 工具链与构建系统支持不完整，ABI 不稳 |

跨标准演进：C++20 通过 P1103R3 引入模块；后续标准修正了部分模块规则，但构建系统与编译器实现仍是落地门槛。

完整示例暂缺：模块需要多个翻译单元和构建系统配合，当前仓库的单文件示例编译流程无法覆盖。
