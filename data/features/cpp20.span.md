---
id: cpp20.span
title: std::span
standard: C++20
kind: library
category: [视图, 内存]
difficulty: 3
status: final
tier: A
compilers: {gcc: "10", clang: "9", msvc: "19.26"}
tags: [连续内存, 非拥有视图, 边界]
alternatives: [指针 + 长度, std::string_view, gsl::span, std::vector]
proposal: P0122R7
last_verified: "2026-10-24"
claims:
  - id: c1
    statement: "返回可写 span<u8> 指向压缩缓冲区"
    evidence: [ev-mold-span-buffer-003]
  - id: c2
    statement: "把重定位表表示为 span<const ElfRel<E>>"
    evidence: [ev-mold-span-relocations-002]
  - id: c3
    statement: "用 span 与 subspan 把缓冲区切成固定大小的块"
    evidence: [ev-mold-span-split-001]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

“一块连续内存 + 长度”是 C 风格接口最常见的参数组合，但两个参数可以彼此不一致：指针有效而长度错误，或长度正确而指针已失效。调用方无法从类型上看出函数是否拥有内存、是否允许写入。模板代码里还经常被迫为 `std::vector`、`std::array`、C 数组分别写重载。

`std::span<T>` 把指针与长度绑定成一个轻量视图。它不拥有数据，可隐式从数组、`std::array`、`std::vector` 构造，并保留元素类型与可写性。它解决的是**接口表达**问题，不是生命周期问题：视图存活多久仍由调用方保证。

## 基础用法

```cpp
#include <array>
#include <iostream>
#include <span>
#include <vector>

int sum(std::span<const int> values) {
  int total = 0;
  for (int value : values) total += value;
  return total;
}

int main() {
  int raw[] = {1, 2, 3};
  std::array<int, 2> arr{4, 5};
  std::vector<int> vec{6, 7};

  std::cout << sum(raw) << '\n';
  std::cout << sum(arr) << '\n';
  std::cout << sum(vec) << '\n';
}
```

完整示例见 [`examples/cpp20/span.cpp`](../../examples/cpp20/span.cpp)：

```text
6
9
13
6
2 3
4
60 70
12
```

三种容器都隐式转成同一个视图类型；函数不需要模板，也不需要为每种容器写重载。

## 进阶用法

**静态长度与动态长度。** `std::span<int, 3>` 在类型里固定长度，适合编译期已知大小的缓冲区；默认的 `std::span<int>` 使用 `std::dynamic_extent`，长度在运行期保存。静态长度版本可以避免长度字段，但构造时要求实际长度匹配。

**`span<const T>` 与 `const span<T>` 不是一回事。** 前者禁止通过视图修改元素；后者只是视图本身不可重新赋值，元素仍可写。接口想表达“只读”时用 `span<const T>`，并把它按值传递。

**`subspan` 返回新的视图。** `s.subspan(offset, count)`、`s.first(n)`、`s.last(n)` 都不复制数据。运行期参数产生动态长度视图；编译期常量参数可以保留静态长度。越界是调用方责任，标准不检查。

**没有边界检查。** `operator[]`、`front`、`back` 在越界或空视图上是未定义行为。C++26 加入 `at()` 才提供抛异常版本；在此之前，需要检查时自己写 `if (i < s.size())`。

**生命周期不会延长。** `std::span<const int> s = std::vector<int>{1, 2, 3};` 中临时 `vector` 在完整表达式结束后销毁，`s` 立即悬垂。不要从临时容器、临时数组或返回局部数组的函数构造长期存活的 `span`。把 `span` 作为函数参数是安全的，因为调用期间实参仍然存活。

**按值传递。** `span` 通常只有指针与长度两个机器字，复制廉价；不要写 `const std::span<T>&`，那会多一层间接并阻止编译器优化。

**与 `std::string_view` 的分工。** `string_view` 专门表示文本，带字符语义与字符串操作；`span<const char>` 只表示字节序列。处理文本用 `string_view`，处理二进制或泛型连续内存用 `span`。

**多维数据用 `std::mdspan`。** C++23 的 `mdspan` 在 `span` 之上增加维度与步长；二维矩阵、图像缓冲区不要用嵌套 `span` 手工计算索引。

## 真实项目证据

| 项目        | 位置                                                                                                                                            | 用法                                            | 为什么典型                                        |
| ----------- | ----------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------- | ------------------------------------------------- |
| rui314/mold | [`src/output-chunks.cc#L880-L892`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/output-chunks.cc#L880-L892) | 用 `span` 与 `subspan` 把缓冲区切成固定大小的块 | 视图切片不复制数据，长度信息随视图传递            |
| rui314/mold | [`src/arch-x86-64.cc#L367-L374`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/arch-x86-64.cc#L367-L374)     | 把重定位表表示为 `span<const ElfRel<E>>`        | 只读接口用 `span<const T>` 表达，调用方无需传长度 |
| rui314/mold | [`src/gdb-index.cc#L637-L642`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/gdb-index.cc#L637-L642)         | 返回可写的 `span<u8>` 指向压缩缓冲区            | 非拥有视图同时表达起始地址、长度与可写性          |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。mold 的构建配置声明 `cxx_std_20`。

## 陷阱与边界

- **悬垂是首要风险。** `span` 不拥有数据；容器重新分配、局部数组销毁、临时对象结束都会让视图失效。
- **不要从临时容器构造长期视图。** 函数返回 `span` 指向局部变量是典型错误；返回 `vector` 或让调用方提供缓冲区。
- **越界不检查。** `s[i]`、`s.front()`、`s.back()` 越界是未定义行为；C++26 之前没有 `at()`。
- **`span<const T>` 不能写，`const span<T>` 可以写。** 把只读意图写成后者会允许修改元素，破坏接口约定。
- **静态长度不是运行时保证。** `span<int, 3>` 的长度在类型里，但从动态视图构造时标准只要求前置条件；违规不会自动抛异常。
- **不要用 `span` 表达所有权。** 需要拥有数据时用 `vector`、`array` 或自定义容器；`span` 只借看。
- **`span` 不保证空终止。** 传给 C API 前确认是否需要 `'\0'`；文本优先 `string_view`，需要 C 字符串时显式复制。
- **异步接口要特别小心。** 把 `span` 存进回调或线程池，必须保证底层内存在回调执行期间仍然存活。

## 库联动

- **mold**：链接器在重定位、输出分块与压缩缓冲区之间大量使用 `span`，把“指针 + 长度”收敛成一个参数，并在切片时不复制数据。
- **fmt / spdlog**：格式化库的范围支持可以接受连续容器；把 `span` 传给 `fmt::join` 可以按元素输出。注意 `span<char>` 不会按字符串处理，仍按字符序列格式化。
- **nlohmann/json**：解析器接受字节容器，`std::as_bytes` 产生的 `span<const std::byte>` 可直接作为输入；生命周期由调用方保证。
- **与 ranges 联动**：`span` 是 `contiguous_range`、`sized_range` 与 `borrowed_range`，可以直接参与 `views::take`、`views::reverse` 等组合。把 `span` 存进视图后仍要注意底层数据寿命。

## 横向对比

| 方案               | 拥有数据 | 携带长度   | 元素类型安全 | 主要代价                       |
| ------------------ | -------- | ---------- | ------------ | ------------------------------ |
| `std::span<T>`     | 否       | 是         | 是           | 生命周期由调用方保证           |
| 指针 + 长度        | 否       | 手动       | 弱           | 两个参数可能不一致             |
| `std::string_view` | 否       | 是         | 仅字符       | 不能表达可变字节或泛型元素     |
| `gsl::span`        | 否       | 是         | 是           | 额外依赖；与标准版行为略有差异 |
| `std::vector<T>`   | 是       | 是         | 是           | 堆分配与所有权转移             |
| `std::array<T, N>` | 是       | 编译期固定 | 是           | 长度固定，不能接受动态输入     |

跨标准演进：C++20 引入 `std::span` 与 `std::as_bytes`；C++23 增加 `std::mdspan` 处理多维视图；C++26 计划加入 `span::at()` 等边界检查接口。当前代码若需要异常式越界检查，应自行封装。
