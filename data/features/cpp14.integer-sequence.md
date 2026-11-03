---
id: cpp14.integer-sequence
title: std::integer_sequence
standard: C++14
kind: library
category: [模板, 编译期计算, 可变参数]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.9", clang: "3.4", msvc: "19.0"}
tags: [integer_sequence, index_sequence, 参数包, tuple, 编译期]
alternatives: [递归模板展开, 手写索引表, C++17 折叠表达式, 运行期循环]
proposal: N3658
last_verified: "2026-11-03"
claims:
  - id: c1
    statement: "用 index_sequence 展开 tuple，把每个元素交给回调"
    evidence: [ev-mp-units-integer-sequence-tuple-001]
  - id: c2
    statement: "用 index_sequence 把 JSON 数组下标展开为 tuple 元素"
    evidence: [ev-nlohmann-integer-sequence-tuple-002]
  - id: c3
    statement: "用 index_sequence_for 展开一组异步任务并逐个启动"
    evidence: [ev-drogon-integer-sequence-tasks-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

可变参数模板把类型包 `Ts...` 保留到模板内部，但包里的每个元素没有下标，无法像数组那样用循环访问。要遍历 `std::tuple`、把一组参数逐个映射到函数、或为每种类型生成一个编号，只能靠递归模板一层层剥离参数，代码冗长、编译期深度也难控制。

`std::integer_sequence<T, Values...>` 是一个只存在于类型里的整数序列。它把 0、1、2 等下标编码成参数包，配合函数模板的参数推导，可以一次性展开所有下标，让编译器生成对应的 `std::get` 或函数调用。标准提案为 N3658，头文件是 `<utility>`。

## 基础用法

`std::integer_sequence<T, 0, 2, 4>` 表示一组编译期整数；`std::index_sequence<Is...>` 是 `T = std::size_t` 的别名，`std::make_index_sequence<N>` 生成 `0..N-1`，`std::index_sequence_for<Ts...>` 按类型包长度生成同样的序列。

<!-- compile -->

```cpp
#include <array>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>

template <typename T, T... Values>
constexpr std::array<T, sizeof...(Values)> to_array(
    std::integer_sequence<T, Values...>) {
  return {{Values...}};
}

template <typename Tuple, typename F, std::size_t... Is>
void for_each_impl(const Tuple& tuple, F&& func, std::index_sequence<Is...>) {
  int unused[] = {0, ((void)func(std::get<Is>(tuple)), 0)...};
  (void)unused;
}

template <typename Tuple, typename F>
void for_each_tuple(const Tuple& tuple, F&& func) {
  for_each_impl(tuple, func,
                std::make_index_sequence<std::tuple_size<Tuple>::value>{});
}

int main() {
  constexpr auto evens = to_array(std::integer_sequence<int, 0, 2, 4, 6>{});
  static_assert(evens.size() == 4, "four values");
  static_assert(evens[3] == 6, "last value");

  int sum = 0;
  for (int value : evens) {
    sum += value;
  }
  std::cout << evens.size() << ' ' << sum << '\n';

  const auto record = std::make_tuple(1, 2.5, std::string("atlas"));
  for_each_tuple(record, [](const auto& value) { std::cout << value << ' '; });
  std::cout << '\n';

  std::cout << std::tuple_size<decltype(record)>::value << '\n';
}
```

输出 `4 12`、`1 2.5 atlas `（末尾有一个空格）与 `3`。`to_array` 把任意整数序列直接展开成 `std::array`；`for_each_tuple` 用 `make_index_sequence` 生成下标包，再在 `for_each_impl` 中展开成对每个元素的调用。完整版本见 [`examples/cpp14/integer_sequence.cpp`](../../examples/cpp14/integer_sequence.cpp)。

## 进阶用法

把序列作为函数参数时，模板参数 `Is...` 会被推导出来；调用方只负责生成序列，展开逻辑集中在被调函数里。这种“生成序列—推导包—一次展开”的三段式是可复用写法，`std::apply` 内部也采用类似结构。

`index_sequence_for<Ts...>` 直接按类型包长度生成下标，适合 tuple 和 variant 这类元素个数由类型决定的容器。`make_index_sequence<N>` 则用于长度已知但元素类型无关的场景，例如把运行期数组复制成 `std::array`。

自定义 `integer_sequence<T, ...>` 可以携带非 `size_t` 的整数。`to_array` 示例用 `int` 序列直接生成数组；也能用 `std::integer_sequence<char, 'a', 'b'>` 在编译期构造字符表。

C++17 的折叠表达式可以简化展开体，但生成下标仍然需要 `index_sequence`。两者是互补关系：序列负责编号，折叠负责把包缩减成一条表达式。

在 C++14 之前，Abseil 与 nlohmann/json 都自带 `integer_sequence` 的兼容实现，用递归和偏特化模拟同样的接口；升级到 C++14 后只需切换到 `std::` 版本。

## 真实项目证据

| 项目          | 位置                                                                                                                                                                                         | 用法                                                  | 为什么典型                                      |
| ------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------- | ----------------------------------------------- |
| mp-units      | [`src/core/include/mp-units/ext/algorithm.h#L167-L177`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/core/include/mp-units/ext/algorithm.h#L167-L177) | 用 `index_sequence` 展开 tuple，把每个元素交给回调    | 序列生成与折叠展开分离，是 tuple 遍历的标准骨架 |
| nlohmann/json | [`single_include/nlohmann/json.hpp#L5230-L5234`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/single_include/nlohmann/json.hpp#L5230-L5234)                | 用 `index_sequence` 把 JSON 数组下标展开成 tuple 元素 | 序列下标直接对应容器位置，避免手写每个字段      |
| drogon        | [`lib/inc/drogon/utils/coroutine.h#L887-L892`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/utils/coroutine.h#L887-L892)           | 用 `index_sequence_for` 展开一组异步任务并逐个启动    | 类型包长度决定任务数，下标在编译期完成分配      |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖 tuple 遍历、序列化与任务批量启动。

## 陷阱与边界

**序列只存在于类型里。** `integer_sequence` 没有运行期迭代器，不能 `for` 遍历；它的用途是让编译器展开参数包，而不是替代普通数组。

**展开顺序不保证。** C++17 之前，花括号初始化列表中的求值顺序没有保证；依赖副作用的展开写法要改用折叠表达式或显式循环。

**空包需要单独处理。** 展开为空时，`func(std::get<Is>(tuple))...` 可能生成空表达式；用额外哨兵元素或偏特化处理 `index_sequence<>`。

**下标越多，实例化越多。** 每个长度生成独立函数模板，过大的序列会显著增加编译时间和二进制体积。

**`index_sequence` 的元素类型固定。** 它等价于 `integer_sequence<std::size_t, ...>`；需要 `int` 或其他类型时直接用 `integer_sequence<T, ...>`。

**不能把序列当运行期容器。** 从运行期变量生成序列需要 `make_index_sequence<N>`，其中 `N` 必须是常量表达式。

**与 C++11 兼容层混用要小心。** 项目自带的 `index_sequence` 与 `std::index_sequence` 是不同类型，模板重载可能产生歧义。

## 库联动

- **`std::tuple`**：用下标序列展开 `std::get`，是 tuple 遍历、序列化和哈希的基础。
- **`std::array`**：把 `integer_sequence` 直接展开成元素列表，编译期生成常量表。
- **`std::variant`**：按 `variant_size` 生成下标，逐个尝试类型分支。
- **`std::apply`**：C++17 用 tuple 元素作为函数实参，内部正是索引序列展开。
- **nlohmann/json**：tuple 反序列化用序列下标读取 JSON 数组的对应位置。
- **Abseil**：`absl::integer_sequence` 为 C++11 项目提供同接口兼容层。

## 横向对比

| 方案               | 需要下标 | 编译期展开 | 代码量       | 主要代价                   |
| ------------------ | -------- | ---------- | ------------ | -------------------------- |
| `integer_sequence` | 是       | 是         | 少           | 需要理解参数包推导         |
| 递归模板展开       | 否       | 是         | 多           | 模板深度大，错误信息难读   |
| 手写索引表         | 是       | 否         | 随元素数增长 | 元素数量变化就要改代码     |
| C++17 折叠表达式   | 否       | 是         | 少           | 只负责缩减，不负责生成下标 |
| 运行期循环         | 是       | 否         | 少           | 不能访问 tuple 的异质元素  |

跨标准演进：C++14 通过 N3658 引入 `integer_sequence` 与 `index_sequence`；C++17 的折叠表达式简化了展开体，但序列生成仍然必要；C++20 的模板 lambda 与 `std::apply` 进一步降低了手写展开的频率，底层机制未变。
