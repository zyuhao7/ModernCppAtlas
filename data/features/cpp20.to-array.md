---
id: cpp20.to-array
title: std::to_array
standard: C++20
kind: library
category: [容器, 语法]
difficulty: 1
status: final
tier: A
compilers: {gcc: "10", clang: "10", msvc: "19.25"}
tags: [to_array, std::array, constexpr, 数组推导]
alternatives: [手写 std::array, C 数组, std::vector, 花括号直接初始化]
proposal: P0325R4
last_verified: "2026-11-12"
claims:
  - id: c1
    statement: "用 std::to_array 把枚举到字符串的映射表初始化为 constexpr 数组"
    evidence: [ev-hyprland-to-array-bind-flags-001]
  - id: c2
    statement: "用 std::to_array 定义要忽略的信号集合，元素类型自动推导"
    evidence: [ev-nghttp2-to-array-signal-set-002]
  - id: c3
    statement: "用 std::to_array 构造字符查找集交给范围查找函数使用"
    evidence: [ev-nghttp2-to-array-terminators-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++ 里想拿到一个 `std::array` 得手写完整类型：`std::array<int, 3> a{1, 2, 3};`，元素个数 `3` 和元素列表是两处独立信息，改列表就要改数字，容易不一致。用裸 C 数组 `int a[]{1, 2, 3}` 能自动推长度，但会退化，传进函数时长度丢失，也没有 `size()`、`begin()` 成员。

CTAD 能简化一部分，但 `std::array` 的推导指引要求所有元素类型一致，且没法从 `std::initializer_list` 直接构造。需要“从一个花括号列表得到一个有完整容器接口、长度自动推导的 `std::array`”时，缺一个统一的入口。

`std::to_array` 就是那个入口：吃一个内建数组或花括号列表，返回 `std::array`，长度和元素类型自动推导，结果是 `constexpr` 可用的值。

## 基础用法

`std::to_array({...})` 从列表推导元素类型与长度；`std::to_array<T>({...})` 显式指定元素类型。返回 `std::array<T, N>`。

<!-- compile -->

```cpp
#include <array>
#include <iostream>

int main() {
  constexpr auto values = std::to_array({1, 2, 3});
  std::cout << values.size() << ' ' << values.back() << '\n';
}
```

输出：

```text
3 3
```

完整版本见 [`examples/cpp20/to_array.cpp`](../../examples/cpp20/to_array.cpp)，六个场景的输出为：

```text
[1] size = 5, front = 1, back = 5
[2] names = red blue
[3] first = red, last = green
[4] is std::array = 1
[5] updated = 4.5
[6] copy size = 5, sum = 6
```

场景 1 从整数列表推出长度为 5 的数组，演示 `size()`、`front()`、`back()`。场景 2 显式指定元素类型为 `std::string_view`，得到字符串表。场景 3 构造 `pair` 列表，用于枚举到名称的映射。场景 4 用 `static_assert` 确认返回类型确实是 `std::array`。场景 5 说明结果是可变左值，元素可写。场景 6 复制数组并求和。

## 进阶用法

**显式元素类型。** 列表里含枚举或需要统一到某个类型时，写 `std::to_array<std::pair<E, std::string_view>>({...})`。不指定时要求所有元素可推导为同一类型，混用字面量和自定义类型会失败。

**接受已有数组。** `std::to_array` 也能吃内建数组：`int raw[]{1, 2, 3}; auto a = std::to_array(raw);` 得到一份拷贝并保留长度，用于把 C 接口返回的数组纳入容器世界。注意它复制元素，不接管原数组。

**离开初始化列表的上下文。** 从函数返回 `std::array`、作为模板非类型实参、参与 `constexpr` 计算时，`std::to_array` 让花括号列表变成一等值，不再需要先声明一个具名数组。

**与 CTAD 的分工。** `std::array` 的 CTAD 需要显式写出元素类型（`std::array{1, 2, 3}` 在 C++17 推不出），`std::to_array` 则能从列表直接推。构造 `std::array` 优先用 `to_array`，修改已有对象时用 `std::array` 自身接口。

**长度是类型的一部分。** 返回的 `N` 由实参个数决定，之后不能增删元素。需要动态长度时用 `std::vector`，`to_array` 只服务固定长度场景。

## 真实项目证据

| 项目     | 位置                                                                                                                                                                  | 用法                                            | 为什么典型                                   |
| -------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------- | -------------------------------------------- |
| Hyprland | [`src/ipc/s1/Commands.cpp#L1167-L1173`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/ipc/s1/Commands.cpp#L1167-L1173)         | 把枚举到字符串的映射表初始化为 `constexpr` 数组 | 显式元素类型，列表长度不写死，改表不必改数字 |
| nghttp2  | [`src/shrpx_signal.cc#L119-L123`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/shrpx_signal.cc#L119-L123)                     | 定义要忽略的信号集合，元素类型自动推导          | 省略模板参数即可得到 `constexpr std::array`  |
| nghttp2  | [`src/shrpx_client_handler.cc#L1306-L1306`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/shrpx_client_handler.cc#L1306-L1306) | 构造字符查找集交给后续范围查找                  | 一行替换裸数组，避免手动维护长度             |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖映射表、信号集合与字符集三种用途。

## 陷阱与边界

**元素类型必须可推导或显式给出。** 空列表 `std::to_array({})` 无法推导长度与类型，会编译失败。至少给一个元素，或用 `std::to_array<T>({})` 配合显式类型——但空列表下长度仍为 0，意义不大。

**它复制元素。** 传入内建数组时得到的是副本，不是视图。大数组会带来一次整体拷贝；需要零拷贝时改用 `std::span`。

**返回类型带 `const` 取决于目标。** `constexpr auto a = std::to_array(...)` 得到 `const std::array`；`auto a = ...` 则是可变副本。需要写成 `static constexpr` 的表才是编译期常量。

**长度嵌入类型，无法在运行期变化。** 想按输入决定长度必须用 `std::vector`。把 `to_array` 当成“方便的 vector 字面量”会很快卡在固定长度上。

**旧标准没有它。** GCC 10、Clang 10、MSVC 19.25 起才提供；C++17 只能用 CTAD 加显式类型，或者自己写推导辅助函数。

## 库联动

- **`std::array`**：`to_array` 返回的就是它，之后可用全套成员接口。
- **`std::span`**：把 `to_array` 的结果交给 `span` 可以得到视图，无需再复制。
- **`std::string_view`**：字符串常量表常用 `std::to_array<std::string_view>({...})`。
- **CTAD**：`std::array` 的类模板实参推导与 `to_array` 互补，前者用于具名构造，后者用于列表推导。

## 横向对比

| 方案                    | 长度推导 | 容器接口 | `constexpr` | 主要限制                       |
| ----------------------- | -------- | -------- | ----------- | ------------------------------ |
| `std::to_array({...})`  | 是       | 是       | 是          | C++20 起；长度固定且需非空列表 |
| `std::array<T, N>{...}` | 否       | 是       | 是          | 元素个数与列表两处需手动一致   |
| 裸 C 数组 `T a[]{...}`  | 是       | 否       | 是          | 会退化，无 `size()` 等成员     |
| `std::vector<T>{...}`   | 是       | 是       | 否          | 运行期分配，非 `constexpr`     |

`std::to_array` 由 P0325R4 进入 C++20，补齐了 `std::array` 长期缺少的“从列表推导”入口。它不改变容器语义，只把“长度要写两遍”这一常见摩擦消掉，让固定长度常量表写起来更紧凑。
