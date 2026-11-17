---
id: cpp20.lerp
title: std::lerp
standard: C++20
kind: library
category: [算法, 性能]
difficulty: 2
status: final
tier: A
compilers: {gcc: "9", clang: "9", msvc: "19.28"}
tags: [lerp, 插值, 线性插值, 动画, 数值计算]
alternatives: [a + t * (b - a), std::midpoint, 手写插值, 逐分量插值]
proposal: P0811R3
last_verified: "2026-11-15"
claims:
  - id: c1
    statement: "动画按 OkLab 色彩空间逐分量调用 std::lerp 完成颜色与透明度插值"
    evidence: [ev-hyprland-lerp-oklab-001]
  - id: c2
    statement: "带单位的 quantity_point 重载内部 using std::lerp 后对数值分量插值"
    evidence: [ev-mp-units-lerp-quantity-point-002]
  - id: c3
    statement: "UI 动画过渡时用 std::lerp 同时对精灵尺寸与位置做插值"
    evidence: [ev-gw2radial-lerp-ui-sprite-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

线性插值是最常见的数值操作之一：给定起点 `a`、终点 `b` 和参数 `t`，求 `a` 到 `b` 之间按比例 `t` 的位置。手写公式 `a + t * (b - a)` 看起来简单，但在浮点计算中有几个隐患：两个端点非常接近时可能损失精度，`t` 超出 `[0, 1]` 时外推行为没有统一约定，`a` 与 `b` 的宽度或符号不一致时还会踩到整型提升与溢出的坑。

`std::lerp` 把这些边界写进标准：它在 `t` 位于 `[0, 1]` 时做线性插值，否则做线性外推，并对浮点计算的舍入误差作出明确规定。P0811R3 把它加入 C++20，定义在 `<cmath>` 中，支持任意算术类型并返回合适的浮点结果。

## 基础用法

`std::lerp(a, b, t)` 在 `t = 0` 时返回 `a`，`t = 1` 时返回 `b`，中间按线性比例过渡。

<!-- compile -->

```cpp
#include <cmath>
#include <iostream>

int main() {
  std::cout << std::lerp(0.0, 10.0, 0.25) << '\n';
}
```

输出：

```text
2.5
```

完整版本见 [`examples/cpp20/lerp.cpp`](../../examples/cpp20/lerp.cpp)，六个场景的输出为：

```text
[1] 2.5
[2] t=0 -> 0, t=1 -> 10
[3] t=2 -> 20
[4] -5
[5] constexpr mid = 1.5
[6] 15
```

场景 1 在四分之一处插值得到 2.5。场景 2 验证两个端点分别复现起点和终点。场景 3 展示 `t = 2` 时的线性外推。场景 4 说明区间可以递减。场景 5 在 `constexpr` 上下文中求中点并用 `static_assert` 校验。场景 6 在更大的区间上取中点。

## 进阶用法

**逐分量插值。** 颜色、坐标、向量通常按分量独立插值。Hyprland 的动画管理器把颜色转换到 OkLab 空间后，对 `l`、`a`、`b` 三个分量各调用一次 `std::lerp`，透明度也单独插值，最后组装回颜色对象。统一使用标准函数让每个分量的语义保持一致。

**自定义数值类型可以复用。** mp-units 为带单位的 `quantity_point` 提供 `lerp` 重载：它先把两个点换算到共同参考单位，再在内部 `using std::lerp;` 后对数值分量插值。这样单位感知的类型也能享受标准的精度保证，而不必另写一套公式。

**同一参数驱动多个值。** UI 过渡常需要让尺寸和位置一起变化。GW2Radial 用同一个 `sizeInterpolant` 分别对精灵尺寸、横向位置和纵向位置调用 `std::lerp`，保证多个视觉属性同步过渡，不会出现尺寸到位而位置滞后。

**支持外推。** `t` 不在 `[0, 1]` 时，`std::lerp` 按同一直线继续延伸，而不是夹到端点。需要夹取时，先对 `t` 调用 `std::clamp`，再传给 `std::lerp`。

**可用于常量表达式。** C++20 起 `std::lerp` 是 `constexpr`，可在编译期计算插值系数或校验固定参数，如示例场景 5 所示。

## 真实项目证据

| 项目                    | 位置                                                                                                                                                                        | 用法                                                           | 为什么典型                                     |
| ----------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------- | ---------------------------------------------- |
| Hyprland                | [`src/animation/AnimationManager.cpp#L65-L75`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/animation/AnimationManager.cpp#L65-L75) | 在 OkLab 空间对三个颜色分量与 alpha 分别调用 `std::lerp`       | 动画插值，逐分量使用标准函数，精度与语义统一   |
| mpusz/mp-units          | [`src/core/include/mp-units/math.h#L471-L485`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/core/include/mp-units/math.h#L471-L485)  | 带单位的 `quantity_point` 重载内 `using std::lerp;` 后插值数值 | 量纲感知类型复用标准实现，先换算共同单位再插值 |
| Friendly0Fire/GW2Radial | [`src/Wheel.cpp#L700-L706`](https://github.com/Friendly0Fire/GW2Radial/blob/3178575f74dc68718b1608faeda5f93947f6a563/src/Wheel.cpp#L700-L706)                               | 同一插值参数驱动精灵尺寸与坐标的多个 `std::lerp` 调用          | UI 过渡动画，多个视觉属性需要同步变化          |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。两条来自生产路径，一条来自带单位数值库的重载，覆盖逐分量插值、自定义类型复用与多属性同步三种用法。

## 陷阱与边界

**`t` 超出 `[0, 1]` 是外推，不是夹取。** 需要把结果限制在两个端点之间时，先 `std::clamp(t, 0.0, 1.0)`。

**返回类型可能比实参宽。** 混合整型与浮点实参时，结果类型由标准规定的公共浮点类型决定，不一定是某个实参的类型。需要固定宽度时显式转换。

**浮点误差不会被完全消除。** `std::lerp` 规定了精度要求，但 `t` 本身的计算误差仍会传播；动画中应接受这一点，或在关键处使用更高精度。

**整数实参会先转换。** 两个整数端点加上浮点 `t` 会按浮点计算；如果希望整数结果，需要自己决定舍入方式。

**NaN 传播。** 任一端点或 `t` 是 NaN 时，结果通常也是 NaN，调用方需要自行处理。

**旧工具链没有它。** GCC 9、Clang 9、MSVC 19.28 起提供。更早版本只能手写 `a + t * (b - a)`。

## 库联动

- **`std::clamp`**：把 `t` 限制在 `[0, 1]`，得到夹取版插值。
- **`std::midpoint`**：求 `t = 0.5` 附近的精确中点，和 `std::lerp` 同属精度友好的数值工具。
- **`std::endian` / `std::byteswap`**：处理底层字节表示，与插值无关，但同属 C++20/23 的 `<bit>` 数值工具族。
- **`constexpr` 求值**：在编译期计算固定插值系数或校验参数范围。

## 横向对比

| 方案              | 外推行为 | 精度保证 | `constexpr` | 主要限制                       |
| ----------------- | -------- | -------- | ----------- | ------------------------------ |
| `std::lerp`       | 线性外推 | 有       | 是          | C++20 起；返回公共浮点类型     |
| `a + t * (b - a)` | 线性外推 | 无       | 是          | 手写公式，端点接近时精度较差   |
| `std::midpoint`   | 不支持   | 有       | 是          | 只求中点，不能任意比例         |
| 夹取后手写插值    | 夹到端点 | 无       | 是          | 需要额外 `clamp`，精度仍无保证 |

`std::lerp` 由 P0811R3 进入 C++20，把线性插值从随手写下的公式变成一个边界明确的标准操作。它区分内插与外推、对浮点精度作出规定，也能被自定义数值类型复用，适合动画、图形与数值计算中反复出现的插值需求。
