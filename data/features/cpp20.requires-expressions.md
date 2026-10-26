---
id: cpp20.requires-expressions
title: requires 表达式
standard: C++20
kind: language
category: [概念, 模板]
difficulty: 3
status: final
tier: B
compilers: {gcc: "10", clang: "10", msvc: "19.30"}
tags: [requires, concepts, 约束, SFINAE, 模板]
alternatives: [SFINAE 与 enable_if, if constexpr 探测, 类型特征特化, 手写检测惯用法]
proposal: P0734R0
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "用 requires 表达式检测成员是否存在，并在 if constexpr 中据此选择分支"
    evidence: [ev-mold-requires-expression-unit-type-001]
  - id: c2
    statement: "用 requires 表达式把静态成员存在性封装成可复用的 concept"
    evidence: [ev-mold-requires-expression-concepts-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++17 之前，探测“某个类型是否支持某个操作”通常要写 SFINAE：`std::void_t`、`decltype`、`enable_if` 和重载一起使用。表达式能编译就选中某个重载，不能编译就替换失败。这种写法能工作，但嵌套深、错误信息长，而且约束的意图藏在类型系统里。

`requires` 表达式把“这些表达式是否合法”直接写成一个布尔值。它不执行表达式，只检查语法和语义有效性，可以放在 `static_assert`、`if constexpr` 或概念定义中使用。

## 基础用法

<!-- compile -->

```cpp
#include <concepts>
#include <cstddef>
#include <iostream>

template <typename T>
concept HasValue = requires(T t) {
  t.value;
};

template <typename T>
concept Sized = requires(const T &t) {
  typename T::value_type;
  { t.size() } -> std::convertible_to<std::size_t>;
};

struct Entry {
  using value_type = int;
  int value;
  std::size_t size() const { return 1; }
};

struct Empty {};

int main() {
  std::cout << HasValue<Entry> << ' ' << HasValue<Empty> << ' '
            << Sized<Entry> << ' ' << Sized<Empty> << '\n';
}
```

`requires(T t) { ... }` 的参数是假设变量，只用于书写检查表达式，不会真的构造对象。`t.value;` 是简单要求，只检查成员访问是否合法；`typename T::value_type;` 是类型要求；`{ t.size() } -> std::convertible_to<std::size_t>;` 是复合要求，同时检查表达式合法且返回类型可转换。

## 真实项目证据

| 项目 | 位置                                                                                                                                    | 用法                                                       | 为什么典型                                                 |
| ---- | --------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------- | ---------------------------------------------------------- |
| mold | [`src/gdb-index.cc#L188-L192`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/gdb-index.cc#L188-L192) | 用 `requires { hdr.unit_type; }` 判断 DWARF 头是否有该字段 | 不同 DWARF 版本字段不同，条件表达式让旧格式仍能参与编译    |
| mold | [`src/elf.h#L1796-L1798`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/elf.h#L1796-L1798)           | 用 `requires { E::R_IRELATIVE; }` 等定义架构能力概念       | 把“某架构是否提供某个静态成员”封装成概念，后续约束直接复用 |

两条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。两者都只用简单要求，正好展示 requires 表达式最基础的用法。

## 陷阱与边界

**requires 表达式不是 requires 子句。** `requires { ... }` 求值为 `bool`，是一个表达式；`template <typename T> requires C<T>` 是约束子句。两者可以组合，但语法位置和作用不同。

**它检查“能否编译”，不检查语义是否正确。** `requires(T t) { t.size(); }` 只要求 `size()` 可调用；返回类型不对、返回垃圾值、运行期抛异常都不影响结果。需要约束返回类型时，写成复合要求 `{ t.size() } -> std::same_as<std::size_t>;`。

**表达式不会被求值。** `requires(T t) { t.pop(); }` 不会真的调用 `pop()`，也不会检查容器是否为空。依赖运行期状态的前置条件，requires 表达式表达不了。

**复合要求可以检查 `noexcept`。** `{ expr } noexcept -> Concept;` 同时要求表达式不抛异常并满足返回类型约束。只写 `{ expr } -> Concept;` 不会检查异常规格。

**类型要求和表达式要求分开写。** `typename T::value_type;` 检查嵌套类型是否存在；去掉 `typename` 后含义会变成表达式检查。两者可以组合，但不要把它们当成同一种要求。

**requires 表达式不能吞掉所有错误。** 它把紧邻的无效表达式变成“不满足”，但模板实例化深处的硬错误仍可能直接报错。约束应尽量写在接口层，而不是依赖 requires 捕获所有实现细节。

**概念约束按从左到右的合取求值。** `C<T> && requires { ... }` 中，左侧不满足时右侧不会继续检查。这个顺序可以用来先排除明显不合适的类型，但要避免把副作用写进约束。

## 横向对比

| 方案                   | 表达位置                              | 错误信息           | 可复用性 | 主要代价                               |
| ---------------------- | ------------------------------------- | ------------------ | -------- | -------------------------------------- |
| SFINAE / `enable_if`   | 返回类型或模板参数                    | 冗长，指向替换失败 | 中       | 语法复杂，约束意图不直观               |
| 检测惯用法（`void_t`） | 类型特征                              | 中等               | 高       | 需要为每种能力写 trait，样板代码多     |
| `if constexpr` 探测    | 函数体                                | 局部较清晰         | 低       | 只能影响当前函数，不能约束重载         |
| requires 表达式        | 概念、`if constexpr`、`static_assert` | 较清晰             | 高       | C++20 起可用；只检查有效性，不检查语义 |

跨标准演进：C++20 通过 P0734R0 引入 concepts 与 requires 表达式，取代了大部分 SFINAE 检测惯用法。截至 C++23，这套核心规则没有变化；后续的约束设施仍建立在 requires 表达式之上。
