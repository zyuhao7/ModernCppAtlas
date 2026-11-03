---
id: cpp11.array
title: std::array
standard: C++11
kind: library
category: [容器, 值语义]
difficulty: 1
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "16.0"}
tags: [std::array, 固定长度, 聚合初始化, 值语义, 容器]
alternatives: [C 数组, std::vector, std::span, std::vector 预留容量]
proposal: N1479
last_verified: "2026-11-03"
claims:
  - id: c1
    statement: "用 std::array 保存固定长度的星期缩写表"
    evidence: [ev-spdlog-array-days-001]
  - id: c2
    statement: "用 std::array 声明编译期大小的哈希桶数组"
    evidence: [ev-abseil-array-graph-hash-002]
  - id: c3
    statement: "用 std::array 定义固定长度的哈希摘要类型"
    evidence: [ev-mold-array-digest-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C 数组在表达式里会退化成指针，传参后丢失长度；不能按值复制或比较；也没有迭代器和边界检查。`std::vector` 解决了这些问题，但只要容量固定，它仍然会单独堆分配一块存储，并额外保存指针、大小和容量。

`std::array<T, N>` 是长度写进类型的固定数组容器：对象本身直接包含 `N` 个元素，没有额外堆分配，也不会退化成指针；同时提供 `size()`、迭代器、`front()`/`back()`、按值复制和字典序比较。标准提案为 N1479。

## 基础用法

长度是模板实参，编译期必须已知。C++11 中 `std::array` 是聚合类型，通常用双层花括号完成初始化；元素个数少于 `N` 时其余元素值初始化。`operator[]` 不做检查，`at()` 越界抛 `std::out_of_range`。

<!-- compile -->

```cpp
#include <array>
#include <iostream>

int main() {
  std::array<int, 5> values{{3, 1, 4, 1, 5}};

  int sum = 0;
  for (int value : values) sum += value;
  std::cout << values.size() << ' ' << sum << ' ' << values.front() << ' '
            << values.back() << '\n';

  values.fill(7);
  std::cout << values[0] << ' ' << values.at(4) << '\n';
}
```

输出 `5 14 3 5` 与 `7 7`。完整版本见 [`examples/cpp11/array.cpp`](../../examples/cpp11/array.cpp)。

## 进阶用法

`std::get<I>(arr)` 与 `std::tuple_size` / `std::tuple_element` 让 `std::array` 可以像 tuple 一样按编译期下标访问，也能参与结构化绑定。C++17 起可以用类模板实参推导写成 `std::array a{1, 2, 3}`，长度由初始化列表推导。

`std::array` 支持 `operator==` 和 `operator<` 的字典序比较，元素类型可比较时即可直接比较两个数组。成员函数 `data()` 返回底层存储指针，便于传给 C 接口；`std::span`（C++20）则可以在不丢失长度的前提下借用这段连续存储。

零长数组 `std::array<T, 0>` 是合法的，`size()` 为 0，但 `front()`/`back()` 不能调用。数组对象通常放在栈上或作为成员内联存放；长度很大时要确认栈空间，必要时改用 `std::vector`。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                                  | 用法                                        | 为什么典型                                                        |
| ------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------- | ----------------------------------------------------------------- |
| spdlog | [`include/spdlog/pattern_formatter-inl.h#L151-L152`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/pattern_formatter-inl.h#L151-L152)                 | 用固定长度表保存星期缩写，按 `tm_wday` 下标 | 表长编译期确定，既保留 C 数组布局又带上长度与迭代器               |
| abseil | [`absl/synchronization/internal/graphcycles.cc#L337-L342`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/synchronization/internal/graphcycles.cc#L337-L342) | 用 `constexpr` 常量作桶数声明成员数组       | 容量来自编译期常量，数组随对象内联存放，避免单独堆分配            |
| mold   | [`src/icf.cc#L79-L81`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/icf.cc#L79-L81)                                                                               | 用 `std::array` 定义固定长度摘要类型        | 长度成为类型的一部分，摘要可按值比较并特化 `std::hash` 作为容器键 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖静态查找表、成员缓冲区和值类型三种形态。

## 陷阱与边界

**长度必须是编译期常量。** `std::array<T, n>` 的 `n` 不能来自运行期输入；需要动态长度时用 `std::vector`。

**`operator[]` 不检查边界。** 越界是未定义行为；边界来自外部输入时改用 `at()`，它在越界时抛 `std::out_of_range`。

**C++11 初始化写法容易多写或漏写花括号。** 聚合初始化需要双层花括号（`{{...}}`）才能在所有编译器上稳定通过；C++14 起单层写法也被广泛接受，但项目内应统一。

**传参仍然会退化成指针吗？** `std::array` 本身按值传递不会退化，但把 `arr.data()` 传给 C 接口后就只剩指针；需要保留长度时把数组按引用传递或改用 `std::span`。

**大数组会占用栈空间。** 作为局部变量或成员时元素内联存放；长度达到数兆字节时可能栈溢出，应改用堆容器。

**`fill`/`swap` 是整体操作。** `fill` 给每个元素赋值，元素类型必须可赋值；`swap` 逐元素交换，成本与长度成正比，不是指针交换。

## 库联动

- **`<array>`**：容器本体与 `get`、`tuple_size`、`tuple_element` 都在此头文件。
- **`<algorithm>`**：`std::sort`、`std::find` 等可直接接收 `begin()`/`end()`；C++20 ranges 也能直接接受 `std::array`。
- **`<tuple>`**：`std::get` 与 tuple 协议让数组可以按编译期下标访问，也能参与结构化绑定。
- **spdlog / abseil / mold**：分别用静态查找表、编译期容量缓冲区与固定长度摘要展示三种典型用法。
- **C++20 `std::span`**：把 `std::array` 作为连续存储借用给不拥有数据的接口，同时保留长度信息。

## 横向对比

| 方案          | 长度来源   | 存储位置 | 是否退化 | 边界检查    | 主要限制                     |
| ------------- | ---------- | -------- | -------- | ----------- | ---------------------------- |
| C 数组        | 编译期     | 内联/栈  | 是       | 无          | 丢失长度，不能按值复制或比较 |
| `std::array`  | 编译期     | 内联/栈  | 否       | `at()` 可选 | 长度不能运行期变化           |
| `std::vector` | 运行期     | 堆       | 否       | `at()` 可选 | 单独分配，容量固定时仍浪费   |
| `std::span`   | 运行期视图 | 不拥有   | 否       | 无          | 不管理生命周期               |

跨标准演进：C++11 通过 N1479 引入 `std::array`；C++14 放宽聚合初始化限制；C++17 支持类模板实参推导；C++20 增加 `std::span` 作为连续存储的非拥有视图。
