---
id: cpp20.numbers
title: 数学常量（std::numbers）
standard: C++20
kind: library
category: [编译期计算, 语法]
difficulty: 1
status: final
tier: A
compilers: {gcc: "10", clang: "11", msvc: "19.25"}
tags: [numbers, pi, 数学常量, constexpr, 精度]
alternatives: [M_PI 宏, 手写字面量, boost::math::constants, 自定义 constexpr 常量]
proposal: P0631R8
last_verified: "2026-11-12"
claims:
  - id: c1
    statement: "渲染代码用 std::numbers::pi 在度与弧度之间换算渐变角度"
    evidence: [ev-hyprland-numbers-angle-degrees-001]
  - id: c2
    statement: "对数域颜色变换用 std::numbers::ln10 换算底数"
    evidence: [ev-hyprland-numbers-log10e-002]
  - id: c3
    statement: "单位库把 std::numbers::pi_v<long double> 作为编译期量纲常量"
    evidence: [ev-mpunits-numbers-pi-constant-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

数学常量长期靠平台宏或手写字面量。`M_PI`、`M_E` 来自 POSIX 而非 C++ 标准，需要先定义 `_USE_MATH_DEFINES` 或 `_GNU_SOURCE` 才可见；它们还是 `double`，想要 `float` 或 `long double` 就得自己加后缀。手写 `3.14159265358979` 更糟：位数随意、精度不明，换个类型又要重抄一遍，容易抄错。

C++20 的 `<numbers>` 提供一组模板变量：每个常量既有 `pi`、`e` 这样的 `double` 实例，也有 `pi_v<T>` 这样的变量模板。常量是 `constexpr`、有明确的数学定义，精度随类型变化，跨平台可用，不再依赖任何宏。

## 基础用法

`std::numbers::pi`、`std::numbers::e`、`std::numbers::sqrt2` 等是 `double` 常量；`std::numbers::pi_v<float>` 得到 `float` 精度。头文件只定义常量，不引入额外运行时开销。

<!-- compile -->

```cpp
#include <iostream>
#include <numbers>

int main() {
  std::cout << std::numbers::pi << ' ' << std::numbers::e << '\n';
  std::cout << std::numbers::pi_v<float> << '\n';
}
```

输出：

```text
3.14159 2.71828
3.14159
```

完整版本见 [`examples/cpp20/numbers.cpp`](../../examples/cpp20/numbers.cpp)，六个场景的输出为：

```text
[1] pi = 3.14159
[2] e = 2.71828, sqrt2 = 1.41421
[3] pi_v<float> = 3.14159274
[4] area = 12.56637
[5] ln10 = 2.302585, log10(e) = 0.434294
[6] radians = 3.141593, cos = -1.000000
```

场景 1 与 2 展示默认 `double` 精度的常用常量。场景 3 用 `pi_v<float>` 得到单精度常量，打印出的 `3.14159274` 揭示了 `float` 能表示的实际值。场景 4 用 `pi` 算圆面积。场景 5 给出对数相关常量：`ln10` 与 `log10e` 互为倒数。场景 6 把 180 度换算成弧度，余弦回到 -1。

## 进阶用法

**按需选择精度。** `pi_v<T>` 是变量模板，`T` 可为 `float`、`double` 或 `long double`。图形、单精度计算用 `pi_v<float>` 可以省去隐式转换和多余的双精度常量；科学计算用 `pi_v<long double>` 提高量级。

**编译期使用。** 常量是 `constexpr`，能出现在 `static_assert`、NTTP 和数组维度里。nlohmann/json、mp-units 一类库把 `pi_v` 直接作为类型参数的一部分。

**完整的常量集合。** 除 `pi`、`e`、`sqrt2` 外，还提供 `sqrt3`、`inv_pi`（1/π）、`inv_sqrtpi`（1/√π）、`ln2`、`ln10`、`log2e`、`log10e`、`egamma`（欧拉-马歇罗尼常数）、`phi`（黄金比例）。这些名字对应明确的数学定义，不再需要自己区分 `log2e` 还是 `ln2`。

**以比率的倒数组织。** `inv_pi`、`inv_sqrtpi` 是为了让“乘倒数”这类常见的性能写法不必先取倒数，同时也避免手写 1/π 时先除后乘带来的额外舍入。

**不是所有数学函数都进了标准库。** `<numbers>` 只补常量；特殊函数、更精细的常数属于 `std::special_functions` 或第三方数学库的范围。

## 真实项目证据

| 项目     | 位置                                                                                                                                                                                                           | 用法                                              | 为什么典型                                       |
| -------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------- | ------------------------------------------------ |
| Hyprland | [`src/render/OpenGL.cpp#L2075-L2075`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/render/OpenGL.cpp#L2075-L2075)                                                      | 用 `std::numbers::pi` 在度与弧度之间换算角度      | 渲染代码反复做角度换算，宏常量在严格环境下不可见 |
| Hyprland | [`src/helpers/cm/ColorManagement.cpp#L403-L413`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/helpers/cm/ColorManagement.cpp#L403-L413)                                | 对数域颜色变换用 `std::numbers::ln10` 换算底数    | 正逆变换共用同一常量，需要一致的精度             |
| mp-units | [`src/core/include/mp-units/framework/unit_magnitude.h#L89-L96`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/core/include/mp-units/framework/unit_magnitude.h#L89-L96) | 把 `std::numbers::pi_v<long double>` 作为量纲常量 | 常量进入类型系统，必须是 `constexpr` 且有精度    |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。两条来自同一图形项目、一条来自单位库，覆盖角度换算、指数变换与类型级常量三种用途。

## 陷阱与边界

**不要用 `numbers::pi` 的精度推断其他类型。** `std::numbers::pi` 是 `double`；需要 `float` 时写 `pi_v<float>`，否则会先构造双精度常量再截断，可能产生一次多余的舍入。

**它与平台宏不是一回事。** `M_PI` 存在与否取决于宏定义；`std::numbers::pi` 是标准设施。两者混用时不要在同一个表达式里依赖同一个值，避免精度语义不清。

**常量不是“无限精确”。** 每个常量都是目标类型下最接近的浮点值。用巨大 `long double` 精度做货币或整数运算仍会累积误差；这类场景应改用整数或定点表示。

**编译期可用不等于精确求值。** `pi_v<T>` 在常量表达式里也是该类型的浮点近似；把它当作 PI 的精确有理数会得到错误结论。

**旧工具链需要回退。** GCC 10、Clang 11、MSVC 19.25 起才完整提供。更早的编译器要退回 `M_PI`、`boost::math::constants` 或自定义 `constexpr` 常量，并注意 `constexpr` 可用性。

## 库联动

- **`<cmath>`**：`std::sin`、`std::exp` 等函数与这些常量配合，替代 `sin(M_PI)` 式的宏用法。
- **`std::numbers` 与 `std::ratio`**：编译期有理数与浮点常量都能作为类型参数，前者适合精确比例、后者适合超越数。
- **`boost::math::constants`**：旧标准或需要高精度常数的替代来源。
- **单位与物理库**：把 `pi` 作为量纲常量嵌入类型（mp-units 即此用法）。

## 横向对比

| 方案                     | 类型可选 | `constexpr` | 依赖宏 | 主要限制                     |
| ------------------------ | -------- | ----------- | ------ | ---------------------------- |
| `std::numbers`           | 是       | 是          | 否     | C++20 起；只有常量不含函数   |
| `M_PI` 等 POSIX 宏       | 否       | 是          | 是     | 非标准，需宏开关，只有双精度 |
| 手写字面量               | 是       | 是          | 否     | 位数随意，易抄错，难以统一   |
| `boost::math::constants` | 是       | 是          | 否     | 引入依赖，命名与精度体系不同 |

`std::numbers` 由 P0631R8 进入 C++20，把散落在宏、编译器扩展和第三方库里的数学常量收进标准。它的价值不在算得更准，而在可移植、带类型、有定义；需要跨精度复用时，`pi_v<T>` 比宏更直接。
