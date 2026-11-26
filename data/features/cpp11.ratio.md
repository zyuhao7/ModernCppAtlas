---
id: cpp11.ratio
title: 编译期有理数（std::ratio）
standard: C++11
kind: library
category: [编译期计算, 元编程]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "17.0"}
tags: [编译期计算, 有理数, chrono, 单位, 元编程]
alternatives: [double 常量, constexpr 函数, 自定义比例类型, 运行时分数]
proposal: N2661
last_verified: "2026-11-26"
claims:
  - id: c1
    statement: "用 std::ratio_divide 计算两个 chrono 周期的比值，转换系数在编译期完成"
    evidence: [ev-fmt-ratio-divide-001]
  - id: c2
    statement: "用 std::ratio 实例比较 chrono 周期，把分钟、小时、天等常见单位映射成后缀"
    evidence: [ev-fmt-ratio-period-002]
  - id: c3
    statement: "把运行时单位比例转换成 std::ratio 类型，再构造 chrono duration"
    evidence: [ev-mpunits-ratio-chrono-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

单位换算需要精确的分数。用 `double` 保存“毫秒是秒的千分之一”会在反复换算后积累舍入误差；用整数保存分子和分母又要把约分、通分、比较全部手写，而且这些操作本可以在编译期完成。时间库尤其依赖这类比例：`std::chrono::duration` 的第二个模板参数就是一个编译期分数。

`std::ratio<Num, Den>`（N2661）把有理数编码进类型：`Num` 和 `Den` 是 `std::intmax_t` 非类型参数，实例化时自动约分，`::num` / `::den` 给出最简分子分母。配套的 `ratio_add`、`ratio_subtract`、`ratio_multiply`、`ratio_divide` 在类型层面做算术，比较模板给出编译期布尔值。运行时不需要任何对象，也不产生浮点误差。

## 基础用法

`std::ratio` 的分子和分母都是编译期常量；`std::milli`、`std::micro`、`std::kilo` 等别名是标准库预定义的比例。算术与比较全部通过类型完成，结果直接用于 `static_assert` 或 `chrono` 的 `period`。

<!-- compile -->

```cpp
#include <chrono>
#include <iostream>
#include <ratio>

int main() {
  using ticks = std::chrono::duration<long long, std::milli>;
  using factor = std::ratio_divide<std::milli, std::micro>;
  static_assert(factor::num == 1000 && factor::den == 1, "milli/micro");
  ticks elapsed(1500);
  std::cout << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()
            << '\n';
}
```

完整示例见 [`examples/cpp11/ratio.cpp`](../../examples/cpp11/ratio.cpp)，六个场景的输出为：

```text
[1] ratio<2, 4> = 1/2
[2] ratio_add<1/2, 1/4> = 3/4
[3] ratio_divide<milli, micro> = 1000/1
[4] milli < centi = 1
[5] milli == ratio<1, 1000> = 1
[6] 1500 ms as seconds = 1
```

场景 1 展示约分：`ratio<2, 4>` 的 `num` / `den` 是 1 和 2。场景 2 用 `ratio_add` 把半秒与四分之一秒相加。场景 3 与 4 分别做除法和比较；场景 5 说明预定义别名与手写分数是同一个类型关系。场景 6 把毫秒时长转换成秒，`duration_cast` 的换算系数正是由 `std::milli` 与 `std::ratio<1>` 的比值得到的。

## 进阶用法

**比例是类型，不是值。** `std::ratio<1, 1000>` 没有运行时状态，所有运算在模板实例化时完成。需要把比例传进函数时传类型或模板参数，而不是构造对象。

**约分发生在实例化。** `ratio<2, 4>` 与 `ratio<1, 2>` 是同一个最简形式；`num` 与 `den` 已经互质。分母不能为零，分子或分母超出 `std::intmax_t` 范围时程序非良构。

**算术模板可能溢出。** `ratio_add` 等模板先通分再约分，中间结果可能超过 `std::intmax_t`；即使最终结果可表示，实现也可能拒绝实例化。大数比例应尽量提前约分，或改用任意精度类型。

**比较模板返回编译期布尔值。** `ratio_equal`、`ratio_not_equal`、`ratio_less`、`ratio_less_equal`、`ratio_greater`、`ratio_greater_equal` 的 `::value` 可以直接用于 `static_assert` 或 `enable_if`。

**chrono 的 period 就是 ratio。** `std::chrono::milliseconds` 是 `duration<long long, std::milli>`。自定义单位时只需提供新的 ratio，`duration_cast` 会据此做精确的整数或浮点换算。

**SI 前缀别名覆盖常用数量级。** `std::kilo`、`std::mega`、`std::milli`、`std::micro`、`std::nano` 等别名避免手写 10 的幂；它们与自己写出的 `ratio<1000>`、`ratio<1, 1000000>` 完全等价。

**不要用它表达运行时比例。** 比例必须是编译期常量；输入来自配置文件或用户参数时，用整数对或浮点数，并在边界处做显式检查。

## 真实项目证据

| 项目           | 固定源码                                                                                                                                                                                                   | 用法                                                    | 为什么典型                                               |
| -------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------- | -------------------------------------------------------- |
| fmtlib/fmt     | [`include/fmt/chrono.h#L162-L171`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/chrono.h#L162-L171)                                                             | 用 `std::ratio_divide` 计算两个 chrono 周期的转换因子   | 先得到精确有理数比例，再决定整数换算路径，避免浮点误差   |
| fmtlib/fmt     | [`include/fmt/chrono.h#L587-L597`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/chrono.h#L587-L597)                                                             | 用 `std::ratio<60>`、`ratio<3600>` 等类型匹配单位后缀   | 单位标签由类型本身携带，运行期只做字符串返回             |
| mpusz/mp-units | [`src/systems/include/mp-units/systems/si/chrono.h#L123-L129`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/systems/include/mp-units/systems/si/chrono.h#L123-L129) | 把单位比例转换成 `std::ratio` 后构造 `chrono::duration` | 库内比例与标准 chrono 的类型系统之间需要精确的有理数桥梁 |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **分母不能为零。** `ratio<1, 0>` 是非良构的；运行时数据不能直接拼进模板实参。
- **中间结果可能溢出。** `ratio_add` / `ratio_multiply` 的通分步骤可能超出 `std::intmax_t`，即使最终结果可表示。
- **`num` / `den` 是最简形式。** 不要假设它们保留原始输入；比较时看值而不是看字面数字。
- **比例不参与运行时计算。** 需要用户输入的比例时，用运行时分数或浮点数，并单独处理除零与精度。
- **浮点周期不是精确比例。** `duration<double, ratio<1, 3>>` 的计数是浮点数，只有 period 本身是精确的 `1/3`。
- **不同 period 的 duration 不能随意混用。** 隐式转换只在不会丢失精度时发生，其余情况必须 `duration_cast`；`ratio` 只描述比例，不替你选择计数类型。
- **工具链下限。** GCC 4.4、Clang 3.0、MSVC 17.0 起提供 `std::ratio` 与 chrono 集成；更早的工具链需要手写比例类型。

## 库联动

- [std::chrono](cpp11.chrono.md)：duration 的 period 就是 `std::ratio`，`duration_cast` 直接使用这里的类型算术。
- [日历与时间点](cpp20.chrono-calendar.md)：C++20 的日历设施延续 duration/period 模型，比例仍然是编译期类型。
- [类型萃取](cpp11.type-traits.md)：`ratio_equal` / `ratio_less` 的结果可以交给 `enable_if`、`static_assert` 等编译期工具。
- [std::numbers](cpp20.numbers.md)：编译期数学常量与编译期比例互补，前者表达实数常量，后者表达精确分数。
- [数值算法](cpp11.algorithms.md)：需要运行时比例或缩放时，在算法边界把 ratio 的分子分母取出来，避免把类型计算带进运行路径。

## 横向对比

| 方案           | 精度       | 计算时机 | 运行时代价 | 主要限制                         |
| -------------- | ---------- | -------- | ---------- | -------------------------------- |
| `std::ratio`   | 精确有理数 | 编译期   | 零         | 只能用于编译期常量，可能中间溢出 |
| `double` 常量  | 浮点近似   | 编译期   | 零         | 反复换算会积累舍入误差           |
| 运行时整数分数 | 精确       | 运行时   | 约分与检查 | 需要手写最大公约数、溢出处理     |
| 自定义比例类型 | 取决于实现 | 编译期   | 零         | 重复标准库已提供的算术与比较     |
| 运行时浮点比例 | 浮点近似   | 运行时   | 浮点运算   | 无法参与模板实例化和单位类型检查 |

`std::ratio` 把“比例”变成类型，适合 chrono、单位库和任何需要精确编译期换算的接口。比例来源是运行时数据，或换算本身不属于程序结构的一部分时，用运行时数值更直接。
