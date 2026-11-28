---
id: cpp23.ranges-to
title: 范围物化（std::ranges::to）
standard: C++23
kind: library
category: [范围, 容器]
difficulty: 2
status: final
tier: A
compilers: {gcc: "14", clang: "17", msvc: "19.34"}
tags: [ranges, 容器构造, 管道, 物化]
alternatives: [迭代器对构造, ranges::copy, 手写循环]
proposal: P1206R7
last_verified: "2026-11-27"
claims:
  - id: c1
    statement: "把视图管道直接物化为 std::vector"
    evidence: [ev-hyprland-ranges-to-001]
  - id: c2
    statement: "用 ranges::to<std::string> 结束字符串变换管道"
    evidence: [ev-hyprland-ranges-to-002]
  - id: c3
    statement: "用 ranges::to<std::vector> 物化 split/transform 结果"
    evidence: [ev-nghttp2-ranges-to-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

ranges 让筛选、变换和切分可以写成惰性视图管道，但视图本身不拥有元素。需要把结果交给只接受容器的接口时，常见写法是先把 `begin()` / `end()` 取出来构造，或者写一个 `for` 循环配合 `push_back`。管道越长，收尾越啰嗦：元素类型要从视图里手工提取，遇到 `transform` 的返回类型推导时还要再写一遍。

C++23 的 `std::ranges::to`（P1206R7）给管道一个物化终点：`view | std::ranges::to<std::vector<int>>()` 直接返回容器。它按目标容器的构造能力选择最佳路径，支持容器模板、具体容器类型和额外构造参数，并能与 `from_range` 标签构造、`insert` 系列接口协作。视图仍保持惰性；只有显式调用 `to` 时才分配并复制或移动元素。

## 基础用法

`std::ranges::to` 既可以放在管道末尾，也可以作为普通函数直接调用。目标类型可以写成 `std::vector<int>`，也可以只写 `std::vector` 让元素类型从范围推导。

<!-- compile -->

```cpp
#include <iostream>
#include <ranges>
#include <vector>

int main() {
  auto values = std::views::iota(1, 5) |
                std::views::transform([](int n) { return n * n; });
#if defined(__cpp_lib_ranges_to_container) && \
    __cpp_lib_ranges_to_container >= 202202L
  auto squares = values | std::ranges::to<std::vector<int>>();
#else
  std::vector<int> squares;
  for (int n : values)
    squares.push_back(n);
#endif
  std::cout << squares.size() << '\n';
}
```

输出 4。完整示例见 [`examples/cpp23/ranges_to.cpp`](../../examples/cpp23/ranges_to.cpp)，三个场景的输出为：

```text
[1] squares = 1 4 9 16 25
[2] fields = 3
[3] first = alpha
```

场景 1 把 `iota(1, 6)` 经过平方变换后一次性收进 `std::vector<int>`；不使用 ranges 的接口时，逐元素 `push_back` 得到相同结果。场景 2 把 CSV 字符串用 `views::split` 切开，每个片段通过 `views::transform` 转成 `std::string`，最后用 `to<std::vector<std::string>>()` 物化，输出字段数 3。场景 3 取物化结果的首个字段，说明得到的是真正拥有元素的容器，而不是仍引用原字符串的视图。

## 进阶用法

**两种目标写法。** `to<std::vector<int>>()` 明确指定容器与元素类型；`to<std::vector>()` 只给容器模板，元素类型从范围的值类型推导。后者更短，但当视图元素是代理类型（例如 `zip` 产生的 `tuple` 引用）时，推导结果可能不符合预期，此时显式写出元素类型更稳妥。

**直接调用与管道等价。** `std::ranges::to<std::vector<int>>(view)` 和 `view | std::ranges::to<std::vector<int>>()` 做同一件事。前者适合嵌套在已有函数调用里，后者与其他视图适配器保持一致。

**构造策略由目标容器决定。** 如果容器有接收 `from_range_t` 标签的构造函数，`to` 优先使用它；否则退回 `insert(begin, end)` 或区间构造函数。目标容器必须能由范围的元素构造，否则在实例化时报错，而不是运行时静默跳过元素。

**额外参数会转发给构造函数。** `ranges::to<std::vector<int>>(rng, alloc)` 可以传入分配器，`to<std::string>()` 可以传入自定义分配器。参数类型和顺序必须匹配目标容器实际存在的构造函数。

**嵌套容器可以分层物化。** `to<std::vector<std::vector<int>>>()` 要求外层范围的元素本身能转换成内层 `std::vector<int>`。对 `split` 这类产生子范围的视图，通常先用 `transform` 把每个子范围转成内层容器，再用外层 `to` 收集。

**注意部分实现的支持边界。** GCC 14 提供 `std::ranges::to`，但容器的 `insert_range` / `assign_range` 成员函数要到 GCC 15 才齐全；只做物化不需要这些成员，`to` 会走其他构造路径。可用 `__cpp_lib_ranges_to_container`（`202202L`）判断库是否提供该设施。

## 真实项目证据

| 项目            | 固定源码                                                                                                                                                              | 用法                                                  | 为什么典型                                         |
| --------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------- | -------------------------------------------------- |
| hyprwm/Hyprland | [`src/render/OpenGL.cpp#L1383-L1387`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/render/OpenGL.cpp#L1383-L1387)             | 坐标变换、`join` 之后用 `to<std::vector<float>>` 收尾 | 渲染层需要连续缓冲区上传 GPU，物化时机和类型都明确 |
| hyprwm/Hyprland | [`src/helpers/MiscFunctions.cpp#L414-L424`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/helpers/MiscFunctions.cpp#L414-L424) | 字符清洗管道以 `to<std::string>` 结束                 | 字符串变换的输出类型由目标容器决定，省去手工拼接   |
| nghttp2/nghttp2 | [`src/util.cc#L986-L990`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/util.cc#L986-L990)                                     | `split` 与 `transform` 后用 `to<std::vector>` 收集    | 配置解析的返回值就是容器，视图到容器的边界一目了然 |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **无限范围会挂起。** `to` 会遍历整个范围并分配内存；`views::iota(0)` 这类无界视图必须先 `take` 或 `take_while` 截断。
- **元素类型推导可能得到代理。** `zip`、`enumerate` 等适配器的元素是引用或代理对象；直接 `to<std::vector>()` 可能产生意料之外的类型，先 `transform` 成目标值类型再物化。
- **视图的生存期必须在物化时有效。** `to` 在调用点复制或移动元素；如果视图引用的是已经销毁的临时对象，物化得到的是悬空值的副本，问题不会因为变成容器而消失。
- **GCC 13 没有该设施。** 即使使用 `-std=c++23`，GCC 13 的标准库仍缺少 `ranges::to`；必须以 `__cpp_lib_ranges_to_container` 做条件编译，或升级到 GCC 14。
- **GCC 14 与 GCC 15 的能力不同。** GCC 14 的 `ranges::to` 可用，但 `insert_range` / `assign_range` 等容器成员函数要到 GCC 15；依赖这些成员的代码需要 `__cpp_lib_containers_ranges`。
- **目标容器必须支持对应构造。** 没有匹配构造函数或 `insert` 的类型无法物化；错误发生在编译期，错误信息通常会指向容器约束。
- **不是零成本转换。** `to` 会分配目标容器并逐元素构造。输入范围有 `size()` 时实现可以预留容量，但不要假设一定只分配一次；对性能敏感的路径仍应测量。

## 库联动

- [范围](cpp20.ranges.md)：`to` 是视图管道的终点，前面的 `filter`、`transform`、`split` 仍按 ranges 的惰性规则工作。
- [从范围构造](cpp23.from-range.md)：目标容器提供 `from_range_t` 构造函数时，`ranges::to` 会优先利用这条路径，两者共享 P1206R7 的容器接口。
- [字符串视图](cpp17.string-view.md)：`split` 常在 `string_view` 上操作，物化前要确认底层字符串的生存期。
- [span](cpp20.span.md)：需要只读连续视图时用 `span`；需要拥有元素并继续增删时用 `ranges::to` 物化到容器。
- [tuple](cpp11.tuple.md)：`zip` 等适配器产生 tuple 风格元素，物化前通常要用 `transform` 把代理转成真正的值类型。

## 横向对比

| 方案                        | 代码长度 | 元素类型推导 | 是否拥有元素 | 主要限制                                 |
| --------------------------- | -------- | ------------ | ------------ | ---------------------------------------- |
| `std::ranges::to`           | 短       | 自动或显式   | 是           | 需要 C++23 标准库，GCC 13 不可用         |
| 迭代器对构造                | 中       | 手写         | 是           | 非 common range 要额外处理，管道易被打断 |
| `ranges::copy` + 插入迭代器 | 中       | 手写         | 是           | 需要先准备容器，元素类型要重复声明       |
| 手写 `for` + `push_back`    | 长       | 手写         | 是           | 分支和提前退出时容易出现遗漏             |
| `from_range` 标签构造       | 短       | 容器推导     | 是           | 只能在构造点使用，不能直接接在管道末尾   |

视图适合表达“怎么算”，容器适合表达“结果归谁所有”。管道需要落地成可增删、可返回的容器时，`std::ranges::to` 是 C++23 起最直接的写法；只需要遍历一次时，保留视图可以避免分配。
