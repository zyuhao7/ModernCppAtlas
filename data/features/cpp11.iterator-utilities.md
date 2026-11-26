---
id: cpp11.iterator-utilities
title: 迭代器工具（std::begin/std::end/std::next/std::prev）
standard: C++11
kind: library
category: [泛型编程, 算法]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.6", clang: "3.0", msvc: "17.0"}
tags: [迭代器, 范围访问, 泛型编程, std::begin, std::distance]
alternatives: [成员 begin/end, 手写指针运算, std::advance, ranges]
proposal: N2930
last_verified: "2026-11-26"
claims:
  - id: c1
    statement: "用 std::begin/std::end 遍历字符数组，数组与容器共用同一组范围边界"
    evidence: [ev-mpunits-iterator-begin-end-001]
  - id: c2
    statement: "泛型容器参数用 std::begin/std::end 取迭代器，不依赖容器的具体类型"
    evidence: [ev-spdlog-iterator-begin-end-002]
  - id: c3
    statement: "用 std::distance 把迭代器位置转换成长度，而不是改写原迭代器"
    evidence: [ev-abseil-iterator-distance-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

泛型代码不能假定容器一定有 `.begin()` / `.end()`，也不能假定范围一定以指针对表示。C 数组没有成员函数，`const` 容器返回的迭代器类型又与 mutable 容器不同；如果每个函数都要求 `Container&`，数组和 `std::initializer_list` 都无法传入。手写 `ptr + size` 则要求随机访问，链表和流式迭代器直接失效。

C++11 的 N2930 把非成员 `std::begin` / `std::end` 放进 `<iterator>`：对容器调用成员函数，对数组返回首尾指针。这样同一段泛型代码可以接收数组、标准容器和用户自定义范围。配套的 `std::next` / `std::prev`（N2246）在不改变原迭代器的前提下前后移动；`std::distance` / `std::advance` 更早就在 C++98 中提供，用来显式表达可能线性的迭代器操作。

## 基础用法

非成员 `std::begin` / `std::end` 是重载集合：容器走 `c.begin()` / `c.end()`，数组走指针算术。`std::next` 返回前进若干步后的副本，`std::prev` 返回后退若干步的副本；`std::distance` 统计两个迭代器之间的元素数量。

<!-- compile -->

```cpp
#include <array>
#include <iostream>
#include <iterator>
#include <vector>

int main() {
  std::array<int, 4> values{{1, 2, 3, 4}};
  int total = 0;
  for (auto it = std::begin(values); it != std::end(values); ++it) {
    total += *it;
  }
  auto second = std::next(values.begin());
  auto last = std::prev(values.end());
  std::cout << total << ' ' << *second << ' ' << *last << ' '
            << std::distance(values.begin(), values.end()) << '\n';
}
```

完整示例见 [`examples/cpp11/iterator_utilities.cpp`](../../examples/cpp11/iterator_utilities.cpp)，六个场景的输出为：

```text
[1] array sum = 10
[2] vector sum = 10
[3] next(begin) = 2, begin = 1
[4] prev(end) = 4
[5] distance(begin, end) = 4
[6] list next(begin, 2) = 3, distance = 2
```

场景 1 与 2 用同一个求和模板分别处理 C 数组和 `std::vector`，调用点只改实参。场景 3 说明 `std::next` 返回新迭代器，原来的 `it` 仍指向首元素；场景 4 用 `std::prev(values.end())` 取尾元素，不必写 `end() - 1`。场景 5 直接计算整个范围的长度。场景 6 在 `std::list` 上前进两步：链表没有随机访问，`std::next` 与 `std::distance` 都按线性步数执行，但接口与随机访问容器保持一致。

## 进阶用法

**用 ADL 惯用法查找 begin/end。** 泛型函数里写 `using std::begin; begin(c);` 可以同时考虑标准重载和用户类型在自身命名空间中提供的 `begin`。直接写 `std::begin(c)` 只调用标准重载，自定义范围若只提供 ADL 版本就会失败。

**`next` / `prev` 不修改实参。** 它们按值接收迭代器，返回移动后的副本。需要原地移动时用 `std::advance`；只是看一眼邻居时用 `next` / `prev` 更安全。

**`distance` 的复杂度取决于迭代器类别。** 随机访问迭代器是常量时间，双向和向前迭代器是线性时间。对输入迭代器调用 `distance` 会消耗传入的副本，原范围本身也会被遍历一次，不能在大范围上无意调用。

**边界不检查。** `std::next(end)`、`std::prev(begin)` 都是未定义行为；`std::next(it, n)` 的步数必须让结果停留在 `[begin, end]` 内。`std::distance` 要求两个迭代器属于同一范围，且 `last` 能通过递增 `first` 到达。

**C++14 增加 `cbegin` / `cend`。** 它们强制以 `const` 方式取范围起点，适合只读遍历；C++11 代码可以写 `std::begin(std::as_const(c))`（`as_const` 是 C++17），或直接接收 `const` 引用。

**C++20 起优先用 ranges。** `std::ranges::begin` / `end` 接受 range 对象并处理哨兵类型；`std::views` 进一步组合惰性视图。非成员迭代器工具仍然有效，尤其是需要显式控制迭代器位置的低层代码。

## 真实项目证据

| 项目              | 固定源码                                                                                                                                                                                       | 用法                                              | 为什么典型                                                           |
| ----------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------- | -------------------------------------------------------------------- |
| mpusz/mp-units    | [`src/core/include/mp-units/framework/unit.h#L823-L826`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/core/include/mp-units/framework/unit.h#L823-L826) | 用 `std::begin` / `std::end` 遍历静态字符数组     | 前缀与分隔符都是数组，非成员边界让拼接代码不依赖数组长度             |
| gabime/spdlog     | [`include/spdlog/fmt/bin_to_hex.h#L64-L69`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/fmt/bin_to_hex.h#L64-L69)                            | 泛型容器参数用 `std::begin` / `std::end` 取迭代器 | `to_hex` 同时接受 vector、string 与数组，调用点不必区分容器类型      |
| abseil/abseil-cpp | [`absl/strings/escaping.cc#L1201-L1205`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/strings/escaping.cc#L1201-L1205)                              | 用 `std::distance` 计算已消费前缀的长度           | 解析器保留当前位置副本，距离计算不修改游标，随机访问迭代器为常量时间 |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **越过边界是未定义行为。** `std::next(end)`、`std::prev(begin)`、`std::next(it, n)` 越界都没有运行时检查。
- **`distance` 可能很贵。** 只有随机访问迭代器是常量时间；链表、流迭代器按元素逐个前进。
- **输入迭代器只能遍历一次。** 对输入迭代器求距离会消耗副本，原范围也可能被读空，之后不能复用。
- **不要混用不同范围的迭代器。** `std::distance` 要求 `last` 能从 `first` 到达，否则行为未定义。
- **`std::begin` 不处理自定义 ADL。** 泛型代码里先 `using std::begin;`，再无限定调用 `begin(c)`。
- **数组退化成指针后失去长度。** 一旦传给 `const int*` 参数，`std::begin` / `std::end` 无从恢复边界；模板按引用接收数组才能保留。
- **`cbegin` / `cend` 是 C++14。** C++11 代码需要只读遍历时显式使用 `const_iterator`。
- **工具链下限。** 非成员 `begin` / `end` 从 GCC 4.6、Clang 3.0、MSVC 17.0 起可用；更早的工具链仍能使用成员函数和 `std::distance`。

## 库联动

- [标准算法](cpp11.algorithms.md)：算法接收迭代器对，`std::begin` / `std::end` 负责把范围转换成这对参数。
- [range-for](cpp11.range-for.md)：范围 for 的展开依赖非成员或成员的 `begin` / `end`，自定义范围需要提供其中之一。
- [size / data / empty](cpp17.size-data-empty.md)：C++17 的非成员范围查询与非成员迭代器访问互补，分别取边界和属性。
- [std::span](cpp20.span.md)：span 把指针加长度封装成范围，迭代器接口与本文工具可以直接对接。
- [ranges](cpp20.ranges.md)：C++20 的 range 概念用哨兵替代同类型 `end`，`std::ranges::begin` 是这套体系的新入口。

## 横向对比

| 方案                 | 引入版本 | 支持数组 | 支持自定义范围  | 主要限制                            |
| -------------------- | -------- | -------- | --------------- | ----------------------------------- |
| 成员 `begin` / `end` | C++98    | 否       | 需要成员函数    | 数组与无成员类型无法使用            |
| 非成员 `std::begin`  | C++11    | 是       | 需要 ADL 惯用法 | 只处理标准重载，不自动查找 ADL      |
| `std::ranges::begin` | C++20    | 是       | 是              | 需要 range 概念，返回类型可能是哨兵 |
| 指针加长度           | C++98    | 是       | 否              | 只适用于连续内存，丢失迭代器抽象    |
| `std::span`          | C++20    | 是       | 连续范围        | 只接受连续内存范围                  |

非成员迭代器工具解决的是“范围边界从哪里来”。它不拥有数据，也不改变元素；需要遍历、定位或测量距离时优先使用标准工具，而不是在调用点重复写指针运算。
