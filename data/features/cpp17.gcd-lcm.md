---
id: cpp17.gcd-lcm
title: 最大公约数与最小公倍数（std::gcd / std::lcm）
standard: C++17
kind: library
category: [算法, 编译期计算]
difficulty: 2
status: final
tier: A
compilers: {gcc: "7", clang: "4", msvc: "19.11"}
tags: [gcd, lcm, 数值算法, constexpr, 约分]
alternatives: [手写辗转相除, __gcd, 运行时循环]
proposal: P0295R0
last_verified: "2026-11-27"
claims:
  - id: c1
    statement: "用 std::gcd 在构造有理数时约分分子分母"
    evidence: [ev-mpunits-gcd-reduce-001]
  - id: c2
    statement: "用两次 std::gcd 在乘法前交叉约分，避免中间溢出"
    evidence: [ev-mpunits-gcd-cancel-002]
  - id: c3
    statement: "用 std::gcd(a, n) == 1 判断模逆元是否存在"
    evidence: [ev-mpunits-gcd-coprime-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

约分、通分、周期对齐和模逆元都需要最大公约数。C++17 之前每个项目都自带一份辗转相除实现：有人返回 `int`，有人对负数处理不一致，有人忘了 `n == 0` 的边界，还有人把实现写成非 `constexpr`，导致编译期比例计算无法复用它。最小公倍数更麻烦，常见写法 `a / gcd(a, b) * b` 顺序稍有不同就会溢出。

C++17 在 `<numeric>` 中加入 `std::gcd` 与 `std::lcm`（P0295R0）。两者对整数类型做统一处理，结果是 `std::common_type_t<M, N>`，并且是 `constexpr`，可以直接用于 `static_assert` 和编译期类型计算。它们不替代溢出检查，但把“算法本身”从每个项目的重复实现提升为标准契约。

## 基础用法

两个函数都接收两个整数，返回公共类型的非负结果。`std::gcd` 用欧几里得算法，`std::lcm` 先除以公约数再相乘，减少中间溢出的机会。

<!-- compile -->

```cpp
#include <iostream>
#include <numeric>

int main() {
  std::cout << std::gcd(48, 18) << ' ' << std::lcm(21, 6) << '\n';
  static_assert(std::gcd(12, 18) == 6);
}
```

输出 `6 42`。完整示例见 [`examples/cpp17/gcd_lcm.cpp`](../../examples/cpp17/gcd_lcm.cpp)，五个场景的输出为：

```text
[1] gcd(48, 18) = 6
[2] lcm(21, 6) = 42
[3] reduce 42/56 = 3/4
[4] coprime(35, 64) = 1
[5] gcd(0, 9) = 9
```

场景 1 与 2 是最小用法。场景 3 把 `std::gcd` 放进约分函数，`42/56` 同时除以 14 得到 `3/4`。场景 4 用 `gcd(a, b) == 1` 判断互质，这是模运算和分数化简里最常见的布尔用法。场景 5 展示零的边界：`gcd(0, 9)` 等于 9。末尾的 `static_assert` 说明这两个函数可以在编译期求值。

## 进阶用法

**结果是公共类型。** `std::gcd` 与 `std::lcm` 的返回类型是 `std::common_type_t<M, N>`，不是左操作数的类型。`std::gcd(12, 18L)` 返回 `long`；把结果写进 `int` 变量前要确认范围。混合有符号与无符号时，公共类型可能是无符号，负数会先按模转换，通常不是想要的结果。

**`constexpr` 与模板计算。** 两个函数从 C++17 起就是 `constexpr`。它们可以出现在 `static_assert`、非类型模板参数的计算和 `std::ratio` 风格的编译期比例化简中；运行时调用则通常被内联成几条算术指令。

**零的语义。** `gcd(a, 0)` 与 `gcd(0, a)` 都等于 `|a|`，`gcd(0, 0)` 返回 0。`lcm(a, 0)` 和 `lcm(0, b)` 返回 0。约分函数要单独处理分母为零，标准库只定义数学上的零值行为，不替你拒绝非法输入。

**在乘法前交叉约分。** 计算 `(a/b) * (c/d)` 时，先算 `g1 = gcd(a, d)`、`g2 = gcd(c, b)`，再用缩小后的数相乘，可以显著降低中间结果溢出概率。这也是 mp-units 比例乘法采用的做法。

**`std::lcm` 仍可能溢出。** 标准实现通常按 `|m| / gcd(m, n) * |n|` 计算，但结果本身超出公共类型时行为未定义。需要处理大数时先用更宽的类型，或改用任意精度整数库；不要指望 `std::lcm` 做溢出检测。

**与分数和比例类型配合。** 约分后的分子分母可用于自定义分数、`std::ratio` 风格的编译期比例，或运行时有理数。标准库只提供最大公约数，不提供“约分后写回”的复合接口，边界检查仍由调用方负责。

## 真实项目证据

| 项目           | 固定源码                                                                                                                                                                             | 用法                                        | 为什么典型                                               |
| -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------- | -------------------------------------------------------- |
| mpusz/mp-units | [`src/core/include/mp-units/bits/ratio.h#L76-L82`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/core/include/mp-units/bits/ratio.h#L76-L82)   | 在 `consteval` 构造函数中用 `std::gcd` 约分 | 比例类型每次构造都要得到最简形式，`std::gcd` 是唯一依赖  |
| mpusz/mp-units | [`src/core/include/mp-units/bits/ratio.h#L97-L103`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/core/include/mp-units/bits/ratio.h#L97-L103) | 乘法前用两次 `std::gcd` 交叉约分            | 先缩小分子分母再调用 `safe_multiply`，把溢出风险降到最低 |
| mpusz/mp-units | [`src/core/include/mp-units/ext/prime.h#L230-L233`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/core/include/mp-units/ext/prime.h#L230-L233) | 用 `std::gcd(a, n) == 1` 检查模逆元是否存在 | 模运算前置条件与最大公约数直接对应，失败路径清晰         |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **结果一定非负。** `std::gcd(-12, 18)` 返回 6。不要假设它保留输入符号；需要符号信息时由调用方单独处理。
- **极值可能触发未定义行为。** 标准要求 `|m|` 和 `|n|` 能在公共类型中表示；`INT_MIN` 的绝对值超出 `int` 范围，`std::gcd(INT_MIN, 0)` 之类调用不保证结果。`std::lcm` 同理，且结果溢出时行为未定义。
- **`std::lcm` 不是防溢出版本。** 即使实现先除后乘，最终结果仍可能超出公共类型。大数场景先用更宽类型或改用大整数库。
- **混合符号与无符号要当心。** 公共类型推导可能把负操作数转换为无符号大数；在调用前统一到有符号类型，或使用 `std::in_range` 检查。
- **C++17 之前不可用。** GCC 7、Clang 4、MSVC 19.11 起提供；旧工具链需要手写辗转相除，别把 `__gcd` 当成可移植替代品。
- **它只处理整数。** 浮点数的最大公约数需要自定义容差和算法；`std::gcd` 的模板约束不接受浮点类型。
- **约分不等于合法性检查。** 分母为零、比值超出可表示范围等业务约束仍要在调用处判断，标准库只负责数学运算。

## 库联动

- [编译期有理数](cpp11.ratio.md)：自定义比例和分数化简直接用 `std::gcd` 把分子分母约到互质。
- [整数比较](cpp20.integer-comparison.md)：混合符号的公共类型推导容易出问题，先用 `std::cmp_*` 或 `std::in_range` 统一语义。
- [位运算工具](cpp20.bit-operations.md)：手写二进制 GCD 时可以配合 `std::countr_zero` 提取因子，但标准 `std::gcd` 已足够处理常规规模。
- [类型萃取](cpp11.type-traits.md)：用 `std::common_type_t` 明确返回类型，或在模板约束里限制整数类型。
- [数值算法](cpp11.algorithms.md)：批量处理分数或周期时，先用 `gcd` 归一化，再交给通用算法排序和合并。

## 横向对比

| 方案                    | 标准来源 | 编译期可用 | 负数处理     | 主要限制                           |
| ----------------------- | -------- | ---------- | ------------ | ---------------------------------- |
| `std::gcd` / `std::lcm` | C++17    | 是         | 结果非负     | 极值与 `lcm` 溢出仍可能未定义      |
| 手写辗转相除            | 任意     | 取决于实现 | 容易不一致   | 每个项目重复实现，边界容易漏       |
| GCC `__gcd`             | 扩展     | 是         | 实现相关     | 非标准、不可移植                   |
| Boost.Integer `gcd`     | 第三方   | 取决于版本 | 有明确文档   | 引入依赖，标准库已覆盖常规需求     |
| 运行时循环              | 任意     | 否         | 由调用方决定 | 无法参与模板参数和 `static_assert` |

需要标准、可移植且能进编译期计算的整数 GCD/LCM 时，直接用 `std::gcd` 和 `std::lcm`。只有在类型远超内置整数范围，或需要多项式、浮点等扩展语义时，才引入专用库。
