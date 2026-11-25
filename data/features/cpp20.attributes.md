---
id: cpp20.attributes
title: C++20 属性（[[likely]]、[[no_unique_address]] 等）
standard: C++20
kind: language
category: [语法, 性能]
difficulty: 2
status: final
tier: A
compilers: {gcc: "9", clang: "12", msvc: "19.28"}
tags: [likely, unlikely, no_unique_address, 属性, 优化提示]
alternatives: [__builtin_expect, 手工重排分支, 空基类优化, 编译器特性宏]
proposal: P0479R5
last_verified: "2026-11-25"
claims:
  - id: c1
    statement: "在解析错误分支标注 [[unlikely]]，提示该路径是冷路径"
    evidence: [ev-glaze-attributes-unlikely-001]
  - id: c2
    statement: "用 [[no_unique_address]] 让空扩展成员与相邻成员重叠"
    evidence: [ev-mold-attributes-nounique-002]
  - id: c3
    statement: "按编译器能力把 [[no_unique_address]] 封装成可移植宏"
    evidence: [ev-fmt-attributes-nounique-003]
review:
  author: "Codex"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 引入了标准属性语法 `[[...]]`，但直到 C++20 才补上两类与性能和布局直接相关的属性：`[[likely]]` / `[[unlikely]]`（P0479R5）用于标注分支的冷热，`[[no_unique_address]]`（P0840R2）用于允许空类型成员与相邻成员共享地址。

它们都不是新语法结构，而是给编译器的提示：分支预测与结构体布局本来由优化器和 ABI 决定，属性把程序员的先验知识显式化。用错不会改变程序的可见行为，但可能让优化落空，所以理解适用边界比记住拼写更重要。

## 基础用法

`[[likely]]` 与 `[[unlikely]]` 放在语句前面，最常见的是 `if` 分支；`[[no_unique_address]]` 放在非静态数据成员声明前。

<!-- compile -->

```cpp
#include <iostream>

int classify(int value) {
  if (value > 0) [[likely]] {
    return 1;
  }
  if (value < 0) [[unlikely]] {
    return -1;
  }
  return 0;
}

struct Empty {};

struct Plain {
  int value;
  Empty tag;
};

struct Compact {
  int value;
  [[no_unique_address]] Empty tag;
};

int main() {
  std::cout << classify(-3) << ' ' << classify(0) << ' ' << classify(5) << ' '
            << (sizeof(Compact) < sizeof(Plain)) << '\n';
}
```

完整示例见 [`examples/cpp20/attributes.cpp`](../../examples/cpp20/attributes.cpp)，输出为：

```text
[1] classify(-3) = -1
[2] classify(0) = 0
[3] classify(5) = 1
[4] sizeof(Plain) = 8, sizeof(Compact) = 4
```

场景 1 与 3 走正负分支，场景 2 走零值分支；属性不改变返回值，只影响代码布局。场景 4 对比带与不带 `[[no_unique_address]]` 的结构体大小，空成员在 `Compact` 中不再单独占位。

## 进阶用法

**属性是提示，不是保证。** 标准只规定编译器“可以”利用这些信息；实际是否重排分支、是否真正重叠存储由实现决定。不要写出依赖 `sizeof(Compact)` 变小的逻辑，也不要依赖 `[[likely]]` 改变分支结果。

**`[[no_unique_address]]` 的语义是地址可重叠。** 对空类型成员，标准允许它与相邻成员拥有相同地址；对非空类型通常没有效果。它常被用来替代“空基类优化 + 继承”的写法，让组合式设计也能省下填充字节。

**跨编译器兼容要检测。** 部分 MSVC 版本忽略标准拼写，需要 `[[msvc::no_unique_address]]`。库通常用 `__has_cpp_attribute` 或特性宏做分支，见证据表中的 fmt 写法。

**属性作用于语句或实体。** `[[likely]]` 放在 `if` 的复合语句前；放在条件表达式里或变量上不符合预期。多个属性可以并列，但顺序不影响语义。

## 真实项目证据

| 项目               | 固定源码                                                                                                                                                                       | 用法                                             | 为什么典型                                        |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------ | ------------------------------------------------- |
| stephenberry/glaze | [`include/glaze/file/raw_or_file.hpp#L35-L38`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/file/raw_or_file.hpp#L35-L38) | 在解析错误分支标注 `[[unlikely]]`                | 反序列化成功是主路径，错误返回应排到冷代码区      |
| rui314/mold        | [`src/mold.h#L313`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/mold.h#L313)                                                              | 用 `[[no_unique_address]]` 压缩 section 扩展成员 | 空扩展类型在默认配置下不应占用每个 section 的空间 |
| fmtlib/fmt         | [`include/fmt/base.h#L166-L174`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/base.h#L166-L174)                                     | 按编译器能力封装 `[[no_unique_address]]` 宏      | 标准拼写在部分 MSVC 上被忽略，需要兼容分支        |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **不改变可观察行为。** 属性只影响优化与布局；若程序正确性依赖某个分支“更可能”，那是逻辑错误，不是属性问题。
- **`sizeof` 结果不是契约。** `[[no_unique_address]]` 是否真的缩小结构体取决于实现与 ABI；测试里可以观察，业务代码不应断言具体大小。
- **对象地址唯一性有例外。** 使用该属性后，两个空成员可能共享地址；依赖“不同对象地址必不相同”的代码需要重新检查。
- **`[[likely]]` 别滥用。** 每个分支都标等于没标；只在确有稳定冷热分布的热点路径上使用，并与性能测量配合。
- **编译器支持不齐。** `[[likely]]` 需要 GCC 9 / Clang 12 / MSVC 19.26，`[[no_unique_address]]` 需要 GCC 9 / Clang 9 / MSVC 19.28；更早工具链要靠 `__builtin_expect` 或空基类优化。
- **与 PGO 的关系。** 有剖面引导优化时，编译器自己的分支权重可能覆盖手写提示；属性更适合没有 PGO 的库代码。

## 库联动

- [属性（C++11）](cpp11.attributes.md) 与 [C++17 属性](cpp17.attributes.md)：`[[nodiscard]]`、`[[fallthrough]]`、`[[maybe_unused]]` 等基础属性。
- **`__builtin_expect` / `__builtin_unpredictable`**：没有 `[[likely]]` 时的编译器内建替代。
- **空基类优化**：`[[no_unique_address]]` 的组合式替代，二者常在同一套类型设计里权衡。
- **`[[nodiscard]]`**：和本页属性一样属于“编译器提示”，但作用于返回值而非控制流。
- **`std::is_empty`**：判断成员类型是否为空，是决定是否加 `[[no_unique_address]]` 的编译期依据。

## 横向对比

| 方案                          | 标准  | 作用对象   | 可移植性 | 主要限制                      |
| ----------------------------- | ----- | ---------- | -------- | ----------------------------- |
| `[[likely]]` / `[[unlikely]]` | C++20 | 语句       | 高       | 只是提示，编译器可忽略        |
| `__builtin_expect`            | 扩展  | 条件表达式 | 低       | 非标准，写法与平台绑定        |
| `[[no_unique_address]]`       | C++20 | 数据成员   | 高       | 布局结果由实现决定            |
| 空基类优化                    | C++98 | 基类       | 高       | 需要改写继承层次              |
| 不加属性                      | —     | —          | 高       | 放弃向编译器传递冷热/布局信息 |

C++20 把分支冷热与空成员布局这两类高频优化需求纳入标准属性，避免直接依赖编译器内建或继承技巧。它们适合性能敏感的解析器、容器和底层数据结构；普通业务代码收益有限，清晰的分支结构通常更重要。
