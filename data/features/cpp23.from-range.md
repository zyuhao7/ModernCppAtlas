---
id: cpp23.from-range
title: 范围构造标记（std::from_range）
standard: C++23
kind: library
category: [容器, 范围]
difficulty: 2
status: final
tier: A
compilers: {gcc: "15", clang: "17", msvc: "19.34"}
tags: [范围构造, 容器, ranges, P1206, 标记类型]
alternatives: [迭代器对构造, ranges::to, 循环插入, assign]
proposal: P1206R7
last_verified: "2026-11-26"
claims:
  - id: c1
    statement: "为自定义字符串容器增加 std::from_range_t 标记构造函数，直接从范围复制"
    evidence: [ev-mpunits-from-range-fixed-string-001]
  - id: c2
    statement: "条件编译的 std::from_range_t 构造函数把范围交给 assign_range"
    evidence: [ev-glaze-from-range-inplace-vector-002]
  - id: c3
    statement: "哈希表用 std::from_range_t 重载接收范围，再转发给迭代器对构造函数"
    evidence: [ev-abseil-from-range-raw-hash-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

标准容器过去只能从迭代器对或 `initializer_list` 构造。迭代器对要求 `first` 与 `last` 类型相同，遇到哨兵类型不同的范围就得先手工拆分；从视图构造时还要写 `v.begin(), v.end()`，既重复又容易在单遍范围上误用。对于长度已知的范围，编译器也无法从迭代器对推断容量。

C++23 的 P1206R7 引入标记构造函数：`container(std::from_range, range)`。`std::from_range_t` 是空标记类型，`std::from_range` 是它的内联常量；容器看到这个标记后直接接收整个 range，可以按 `ranges::size` 预留容量，也允许 `iterator` 与 `sentinel` 类型不同。同一提案还加入 `insert_range`、`assign_range` 与 `std::ranges::to`。

## 基础用法

标记构造把范围作为一个整体传入，调用点不再拆出首尾迭代器。标记类型负责与普通构造函数消歧；范围的值类型必须能转换成容器元素类型。

<!-- compile -->

```cpp
#include <array>
#include <ranges>
#include <vector>
#include <version>

int main() {
  std::array<int, 3> values{{1, 2, 3}};
#ifdef __cpp_lib_containers_ranges
  std::vector<int> copied(std::from_range, values);
#else
  std::vector<int> copied(values.begin(), values.end());
#endif
  return copied.size() == 3 ? 0 : 1;
}
```

完整示例见 [`examples/cpp23/from_range.cpp`](../../examples/cpp23/from_range.cpp)，四个场景的输出为：

```text
[1] vector from array = 1 2 3 4
[2] vector size = 4
[3] list from vector = 1 2 3 4
[4] list front/back = 1 4
```

场景 1 与 2 从 `std::array` 直接构造 `std::vector`，长度由范围本身提供。场景 3 再把 vector 复制进 `std::list`，两次构造都只传范围和标记。场景 4 检查结果的边界元素。示例用 `__cpp_lib_containers_ranges` 守卫：库没有实现完整的容器范围接口时退回迭代器对构造，两种路径输出相同。

## 进阶用法

**标记参数必须放在第一位。** `std::from_range` 是构造函数和成员函数的第一个实参，用来把范围重载与 `(count, value)`、`(first, last)`、`initializer_list` 等重载区分开。

**单遍范围也能消费。** 范围构造按 `input_range` 的语义遍历一次，不要求 `begin` 与 `end` 类型相同，也不要求随机访问。传入生成器或过滤视图时，容器直接消费序列，不必先落地到临时容器。

**长度已知时可预留容量。** 标准容器可以从 sized range 取得元素数量并一次性分配；与先 `reserve` 再循环 `push_back` 相比，范围构造把意图和异常安全都交给容器实现。

**成员接口同样有范围版本。** `insert_range(pos, rg)` 在指定位置插入整个范围，`assign_range(rg)` 替换全部内容。它们与构造函数共享标记类型和值类型约束。

**`ranges::to` 适合管道。** `views | std::ranges::to<std::vector>()` 在表达式末尾构造容器；标记构造适合已经拿到范围对象的场合。两者都由 P1206 引入，但入口不同。

**不是零拷贝。** 范围构造把元素复制或移动进容器，不借用源范围。源范围是临时对象时，构造完成后即可销毁；源元素需要保留所有权时，显式传 `std::move` 或使用 `std::ranges::to` 的移动语义。

**库支持分阶段。** cppreference 记录 GCC 15 才提供完整的容器范围成员函数，GCC 14 只有 `ranges::to`；Clang 17、MSVC 19.34 起提供该接口。需要兼容更早工具链时，用特性宏守卫并保留迭代器对回退。

## 真实项目证据

| 项目               | 固定源码                                                                                                                                                                                             | 用法                                              | 为什么典型                                                   |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------- | ------------------------------------------------------------ |
| mpusz/mp-units     | [`src/core/include/mp-units/ext/fixed_string.h#L104-L110`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/core/include/mp-units/ext/fixed_string.h#L104-L110)   | 为固定长度字符串增加 `std::from_range_t` 构造函数 | 自定义容器用标记重载接收任意输入范围，调用点不必拆 begin/end |
| stephenberry/glaze | [`include/glaze/containers/inplace_vector.hpp#L557-L564`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/containers/inplace_vector.hpp#L557-L564) | 条件编译的标记构造函数把范围交给 `assign_range`   | 只有库声明容器范围能力时才暴露重载，保持旧工具链可编译       |
| abseil/abseil-cpp  | [`absl/container/internal/raw_hash_set.h#L2578-L2585`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/container/internal/raw_hash_set.h#L2578-L2585)        | 哈希表用 `std::from_range_t` 重载接收范围并转发   | 标准容器的范围构造用标记类型消歧，底层仍复用迭代器对实现     |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **需要完整的 C++23 库支持。** 仅升级语言模式不够，标准库必须实现 `__cpp_lib_containers_ranges`；否则标记构造函数不存在。
- **标记不是适配器。** `std::from_range` 只用于选择重载，不会把任意对象转成 range；实参必须满足 `input_range`。
- **构造会复制或移动元素。** 源范围不会被容器接管；持有元素的视图不能靠构造延长其底层存储寿命。
- **单遍范围只能读一次。** 范围构造消费输入序列，构造后不要再次遍历同一个单遍范围。
- **值类型必须可转换。** 元素引用要能转换成容器 `value_type`；只可比较或只可哈希的类型不能直接拷入。
- **重载消歧依赖第一个参数。** 把标记放到后面的位置不会匹配范围构造函数，可能意外落到迭代器对或其他重载。
- **`ranges::to` 与标记构造不是同一入口。** 前者是管道适配器，后者是构造/成员函数；错误混用会得到编译错误而不是隐式转换。
- **工具链下限。** cppreference 记录 GCC 15、Clang 17、MSVC 19.34 起完整支持；更早版本需要迭代器对回退。

## 库联动

- [ranges](cpp20.ranges.md)：范围构造接收 range 对象，视图、哨兵和惰性求值都由 ranges 体系提供。
- [std::to_array](cpp20.to-array.md)：固定的编译期数组转换与运行时范围构造互补，分别解决定长和动态容器。
- [size / data / empty](cpp17.size-data-empty.md)：范围构造利用 sized range 的 `size` 预留容量，非成员查询与标记构造共享同一组范围属性。
- [std::span](cpp20.span.md)：span 是非拥有连续视图，范围构造把 span 的内容复制进拥有容器，两者生命周期语义相反。
- [std::format](cpp20.format.md)：格式化结果常以范围形式产生，再交给标记构造落进容器；管道写法可以用 `ranges::to` 连接。

## 横向对比

| 方案                            | 引入版本 | 支持不同首尾类型 | 可预留容量 | 主要限制                               |
| ------------------------------- | -------- | ---------------- | ---------- | -------------------------------------- |
| `(first, last)` 构造            | C++98    | 否               | 否         | 单遍范围与哨兵需要手工处理             |
| `initializer_list` 构造         | C++11    | 不适用           | 是         | 只接受同类型常量列表，运行期范围不适用 |
| `std::from_range` 构造          | C++23    | 是               | 是         | 需要标准库实现完整容器范围接口         |
| `insert_range` / `assign_range` | C++23    | 是               | 视实现     | 只修改已有容器，不负责首次构造         |
| `views \| ranges::to<C>`        | C++23    | 是               | 是         | 需要 ranges 管道，构造点位于表达式末尾 |

`std::from_range` 把“从整个范围构造容器”变成一条明确的重载。范围已经是对象、或需要让容器自行预留容量时用它；范围还在视图管道里时，`ranges::to` 更自然。
