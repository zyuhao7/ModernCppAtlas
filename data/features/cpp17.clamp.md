---
id: cpp17.clamp
title: std::clamp
standard: C++17
kind: library
category: [算法, 性能]
difficulty: 1
status: final
tier: A
compilers: {gcc: "7", clang: "3.9", msvc: "19.0"}
tags: [clamp, 限幅, 范围约束, 边界检查]
alternatives: [std::min/std::max, 手写 if, std::ranges::clamp, 自定义限幅函数]
proposal: P0025R1
last_verified: "2026-11-12"
claims:
  - id: c1
    statement: "HTTP 日期格式化后用 std::clamp 把写入长度限制在缓冲区内"
    evidence: [ev-drogon-clamp-http-date-001]
  - id: c2
    statement: "把输入设备报告的绝对指针坐标 clamp 到归一化区间 [0, 1]"
    evidence: [ev-hyprland-clamp-absolute-coords-002]
  - id: c3
    statement: "JMESPath 切片用 std::clamp 统一约束方向相关的下标区间"
    evidence: [ev-glaze-clamp-jmespath-index-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

把值限制在区间内是极常见的需求：音量、透明度、进度、坐标、数组下标都要落在上下界之间。C++17 之前只能写成 `std::min(std::max(value, lo), hi)`，或者展开成一对 `if`。嵌套的 `min` / `max` 读起来是“先取大再取小”，与“把值夹住”的意图并不一致；手写分支则容易把上下界顺序写反，或者只处理一边。

`std::clamp(value, lo, hi)` 直接表达“返回 value、lo、hi 中位于中间的那个”：低于下界返回 `lo`，高于上界返回 `hi`，否则原样返回 `value`。它只做一次比较链，返回值是三个参数之一的引用，不复制对象。

## 基础用法

`std::clamp` 要求三个参数类型一致，返回 `const T&`。下界必须不大于上界，否则行为未定义。

<!-- compile -->

```cpp
#include <algorithm>
#include <iostream>

int main() {
  std::cout << std::clamp(15, 0, 10) << ' ' << std::clamp(-5, 0, 10) << '\n';
}
```

输出：

```text
10 0
```

完整版本见 [`examples/cpp17/clamp.cpp`](../../examples/cpp17/clamp.cpp)，六个场景的输出为：

```text
[1] high = 10, low = 0, inside = 5
[2] 0 1 0.25
[3] size = 4
[4] clamp = 10, min(max()) = 10
[5] version = 2.0
[6] samples = 0 0 37 100 100
```

场景 1 展示三种位置：高于上界、低于下界、区间内。场景 2 把浮点数夹到 `[0, 1]`，负数与超过 1 的值分别落到两端。场景 3 显式指定 `std::size_t`，让整数参数统一到无符号类型。场景 4 用 `min(max(...))` 做对照，结果与 `clamp` 相同。场景 5 对自定义 `Version` 类型使用默认的 `operator<`。场景 6 把一组传感器读数逐个夹到 `[0, 100]`。

## 进阶用法

**自定义比较器。** 第四个参数是二元谓词，`std::clamp(value, lo, hi, comp)` 按 `comp` 判断顺序。适用于降序区间、大小写不敏感比较，或模拟器内部自定义的优先级关系。谓词必须满足严格弱序。

**显式模板参数。** 三个参数类型不一致时，可以写成 `std::clamp<std::size_t>(7, 1, 4)`，让实参先转换到统一类型。隐式混用有符号与无符号整数会产生转换警告，显式指定更安全。

**返回引用，不返回值。** 结果是三个实参之一的 `const T&`。用它初始化新对象会复制；把引用保存下来则要保证被引用的对象仍然存活。不要返回指向临时边界的引用。

**常量求值。** `std::clamp` 从 C++17 起就是 `constexpr`，可以出现在 `static_assert` 与常量初始化中。例如把编译期配置夹到合法范围后再用作数组长度。

**C++20 的范围算法。** `std::ranges::clamp` 提供相同的语义，并参与投影与概念约束；一般代码用 `std::clamp` 即可，泛型代码需要投影时再换用范围版本。

## 真实项目证据

| 项目     | 位置                                                                                                                                                                         | 用法                                     | 为什么典型                                 |
| -------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------- | ------------------------------------------ |
| drogon   | [`lib/src/Utilities.cc#L1115-L1116`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/src/Utilities.cc#L1115-L1116)               | 格式化 HTTP 日期后把写入长度夹到合法下标 | 长度立即用于 `resize`，边界必须一次写对    |
| Hyprland | [`src/pointer/PointerManager.cpp#L936-L939`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/pointer/PointerManager.cpp#L936-L939)      | 把绝对指针坐标夹到归一化区间 `[0, 1]`    | 外部设备输入不可信，下游布局计算要求有界值 |
| glaze    | [`include/glaze/json/jmespath.hpp#L704-L707`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/json/jmespath.hpp#L704-L707) | 切片下标换算后夹到方向相关的上下界       | 上下界随步长方向变化，统一收口避免重复判断 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处分别覆盖缓冲区长度、外部输入坐标和解析器下标，都是生产路径。

## 陷阱与边界

**下界不能大于上界。** `std::clamp(v, 10, 0)` 违反前置条件，行为未定义。边界来自运行时数据时，先排序、交换，或用断言固定不变量。

**浮点 NaN 不受约束。** 默认比较器下 NaN 与任何值比较都为假，`clamp(NaN, lo, hi)` 会返回 NaN。需要处理 NaN 时在调用前单独判断，Hyprland 的坐标处理就是先检查 `std::isnan`。

**返回的是引用。** 把 `clamp` 的结果绑定到 `const T&`，再让边界临时对象离开作用域，引用就会悬空。跨语句保存时应复制成值。

**三个参数必须是同一类型。** 字面量混用会触发模板推导失败或隐式转换。`clamp(1, 0.0, 1.0)` 无法推导出单一 `T`；显式指定类型或统一字面量后缀。

**不要用 `clamp` 替代验证。** 它只改变值，不报告“输入越界”。协议、配置等需要拒绝非法输入的场景，应显式检查并报错，而不是悄悄把值压回范围。

**性能不是重点。** 它通常被内联为两次比较，与手写分支等价。真正的收益是可读性与边界一致性。

## 库联动

- **`std::min` / `std::max`**：两个值之间取极值；`clamp` 处理三点区间，二者常在同一段代码里配合。
- **`std::ranges::clamp`**（C++20）：范围算法版本，支持投影，适合泛型约束。
- **`std::chrono`**：时长、时间点只要可比较，就能用 `clamp` 限制在调度窗口内。
- **数值与图形代码**：透明度、音量、坐标、颜色通道普遍需要归一化区间，`clamp` 是最直接的表达。

## 横向对比

| 方案                            | 语句数 | 边界表达       | 返回引用 | 主要限制                     |
| ------------------------------- | ------ | -------------- | -------- | ---------------------------- |
| `std::clamp(v, lo, hi)`         | 1      | 区间整体       | 是       | 要求 `lo <= hi`；参数同类型  |
| `std::min(std::max(v, lo), hi)` | 1      | 先取大再取小   | 是       | 上下界顺序容易读错           |
| 手写 `if` 分支                  | 2–3    | 逐条条件       | 可复制   | 容易漏掉一侧或写反边界       |
| `std::ranges::clamp`            | 1      | 区间整体加投影 | 是       | C++20 起；需要范围算法头文件 |

`std::clamp` 由 P0025R1 进入 C++17，是标准库对“区间限幅”这一手写模式的直接收编。它不引入新能力，但把边界条件集中到一个调用点，审计时更容易确认上下界没有被写反。
