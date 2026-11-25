---
id: cpp17.logical-traits
title: 逻辑类型特征（std::conjunction 等）
standard: C++17
kind: library
category: [类型工具, 元编程]
difficulty: 3
status: final
tier: A
compilers: {gcc: "6.1", clang: "3.8", msvc: "19.0"}
tags: [conjunction, disjunction, negation, 短路求值, 类型特征]
alternatives: [手写递归特化, enable_if 嵌套, C++20 concepts, 折叠表达式]
proposal: P0013R1
last_verified: "2026-11-25"
claims:
  - id: c1
    statement: "用 disjunction_v 判断两个方向中任一隐式转换是否成立"
    evidence: [ev-abseil-logical-traits-uniform-001]
  - id: c2
    statement: "用 conjunction 展开包，要求 variant 的每个备选类型都可格式化"
    evidence: [ev-fmt-logical-traits-variant-002]
  - id: c3
    statement: "用 conjunction_v 组合 is_enum 与多个 negation 条件"
    evidence: [ev-abseil-logical-traits-checkop-003]
review:
  author: "Codex"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

模板里经常需要组合多个布尔类型特征：所有条件都成立、任意一个成立、或者取反。C++11 时代只能用 `std::enable_if` 嵌套，或者手写递归模板把参数包折叠起来；写法冗长，而且很难表达“短路实例化”——前一个条件不成立时就不该再去求值后面的特征。

C++17 的 `std::conjunction`、`std::disjunction`、`std::negation`（P0013R1）把这三类组合标准化。它们都带有 `::value` 与 C++17 的 `_v` 变量模板，并且按顺序短路：`conjunction` 遇到第一个 `false` 就停止实例化后续类型，`disjunction` 遇到第一个 `true` 就停止。

## 基础用法

`conjunction<B...>` 在所有 `B::value` 为真时为真，`disjunction<B...>` 在任意一个为真时为真，`negation<B>` 对单个特征取反。空参数包时，`conjunction<>` 是 `true_type`，`disjunction<>` 是 `false_type`，符合逻辑恒等元。

<!-- compile -->

```cpp
#include <iostream>
#include <type_traits>

int main() {
  std::cout << std::conjunction_v<std::is_integral<int>,
                                  std::is_signed<int>>
            << ' '
            << std::conjunction_v<std::is_integral<double>,
                                  std::is_signed<double>>
            << ' '
            << std::disjunction_v<std::is_integral<int>,
                                  std::is_floating_point<int>>
            << ' ' << std::negation_v<std::is_pointer<int>> << '\n';
}
```

完整示例见 [`examples/cpp17/logical_traits.cpp`](../../examples/cpp17/logical_traits.cpp)，输出为：

```text
[1] conjunction<is_integral<int>, is_signed<int>> = 1
[2] conjunction<is_integral<double>, is_signed<double>> = 0
[3] disjunction<is_integral<int>, is_floating_point<int>> = 1
[4] negation<is_pointer<int>> = 1
```

场景 1 两个条件都成立，结果为真；场景 2 中 `double` 不是整型，`conjunction` 为假；场景 3 整型满足第一个条件，`disjunction` 为真；场景 4 对 `is_pointer` 取反，得到真。

## 进阶用法

**短路实例化。** 短路不只是求值顺序，还意味着后续模板不会被实例化。可以先用一个便宜的布尔特征挡住会触发硬错误的表达式，再在后面检查成员是否存在。这是手写递归特化最难正确实现的细节之一。

**用 `_v` 变体直接拿布尔值。** `conjunction_v<...>` 等价于 `conjunction<...>::value`，在 `enable_if_t`、`if constexpr` 与 `static_assert` 中更简洁。需要类型本身（例如作为基类）时仍用不带 `_v` 的形式。

**把参数包折叠成条件。** `fmt` 用它检查 `std::variant` 的每个备选类型是否可格式化；把一个包展开成多个谓词后交给 `conjunction`，比递归模板短得多，也比手写 fold 表达式更容易读。

**与 `enable_if` 配合做约束。** `std::enable_if_t<std::conjunction_v<...>, T>` 是 C++17 里常见的 SFINAE 形式；C++20 之后通常改用 concept 重写。

## 真实项目证据

| 项目              | 固定源码                                                                                                                                                                            | 用法                                                     | 为什么典型                                     |
| ----------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------- | ---------------------------------------------- |
| abseil/abseil-cpp | [`absl/random/internal/uniform_helper.h#L77-L81`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/random/internal/uniform_helper.h#L77-L81) | 用 `disjunction_v` 判断任一方向的隐式转换是否成立        | 两个参数只要有一个能无损转换就应参与重载       |
| fmtlib/fmt        | [`include/fmt/std.h#L154-L157`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/std.h#L154-L157)                                            | 用 `conjunction` 展开包，检查 variant 的每个备选类型     | 包展开后折叠成单一条件，全部可格式化才允许输出 |
| abseil/abseil-cpp | [`absl/log/internal/check_op.h#L382-L387`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/log/internal/check_op.h#L382-L387)               | 用 `conjunction_v` 组合 `is_enum` 与多个 `negation` 条件 | 多个互斥条件各占一行，重载筛选的意图清晰       |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **短路依赖参数顺序。** 把会触发硬错误的特征放在前面，短路就失去保护作用；需要保护时，把“守卫”条件放在参数包最左边。
- **参数必须是类型特征。** `conjunction` 读取每个参数的 `::value`，传普通布尔值或表达式不会按预期工作；布尔表达式应直接用 `&&` / `||` 或折叠表达式。
- **`negation` 只接受一个参数。** 需要对多个条件取反时，先用 `disjunction` / `conjunction` 组合，再套 `negation`。
- **空包语义容易记反。** `conjunction<>` 为真、`disjunction<>` 为假；把它们当作“全称/存在量词”的恒等元来记。
- **不是运行时短路。** 这些是编译期类型组合，运行时分支仍要用 `if` / `if constexpr`；二者概念不同，不能互相替代。
- **C++20 起有更直接的写法。** 能写成 concept 或 `requires` 表达式时优先用新语法；本页设施更多出现在需要精确保留 `enable_if` 语义的库代码里。

## 库联动

- [类型特征（type_traits）](cpp11.type-traits.md)：`is_integral`、`is_signed` 等被组合的底层谓词。
- [std::void_t](cpp17.void-t.md)：常与这些逻辑特征一起做成员检测。
- [if constexpr](cpp17.if-constexpr.md)：编译期分流的另一位主角，通常先用逻辑特征算出条件。
- **concepts**：C++20 的 `std::integral`、`std::same_as` 等概念可以替代本页的大部分组合场景。
- **折叠表达式**：对布尔值做 `&&` / `||` 折叠，是本页设施在运行期/常量表达式层面的对应物。

## 横向对比

| 方案             | 引入版本 | 短路实例化 | 可读性 | 主要限制               |
| ---------------- | -------- | ---------- | ------ | ---------------------- |
| `conjunction` 等 | C++17    | 是         | 高     | 只接受类型特征         |
| 手写递归特化     | C++11    | 可实现     | 低     | 冗长，短路逻辑易写错   |
| `enable_if` 嵌套 | C++11    | 是         | 低     | 条件一多就难以阅读     |
| 折叠表达式       | C++17    | 否         | 中     | 作用于值，不是类型特征 |
| C++20 concepts   | C++20    | 是         | 高     | 需要 C++20 工具链      |

`std::conjunction`、`std::disjunction`、`std::negation` 由 P0013R1 进入 C++17，把手写的布尔特征组合变成标准设施，并提供可靠的短路实例化语义。它们适合写重载约束、检测成员与组合类型谓词；在新代码里通常让位给 concept，但仍广泛存在于库内部的 `enable_if` 表达式里。
