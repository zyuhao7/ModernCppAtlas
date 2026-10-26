---
id: cpp20.abbreviated-templates
title: 缩写函数模板（abbreviated function templates）
standard: C++20
kind: language
category: [模板, 泛型]
difficulty: 2
status: final
tier: A
compilers: {gcc: "10", clang: "10", msvc: "19.29"}
tags: [缩写模板, auto 参数, 泛型, 约束, concepts]
alternatives: [显式模板参数, 泛型 lambda, 普通重载]
proposal: P1141R2
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "在已写出元素类型的函数模板中，用 const auto& 接收任意可遍历容器"
    evidence: [ev-mold-abbrev-template-append-001]
  - id: c2
    statement: "用单个 auto& 参数定义适用于任意容器的排序辅助函数"
    evidence: [ev-mold-abbrev-template-sort-002]
  - id: c3
    statement: "用两个 auto 参数分别推导容器与比较器类型"
    evidence: [ev-mold-abbrev-template-sort-compare-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

一个只依赖 `begin()`、`end()` 或比较操作的辅助函数，过去要么写完整模板头，要么为每种容器重载：

```cpp
template <typename Container> void sort(Container &vec);
```

类型名只出现一次，模板参数却要单独命名。C++20 允许在函数参数位置写 `auto`：`void sort(auto &vec)`。编译器为每个 `auto` 参数隐式引入一个模板参数，函数仍然是函数模板，调用方式不变。

`auto` 前可以加概念，形成 `std::integral auto`、`std::ranges::range auto` 这类约束；这比在 `template` 头里写 `requires` 更贴近参数，也让重载选择直接受约束控制。

## 基础用法

<!-- compile -->

```cpp
#include <algorithm>
#include <iostream>
#include <vector>

template <typename T>
void append(std::vector<T> &x, const auto &y) {
  x.insert(x.end(), y.begin(), y.end());
}

void sort(auto &vec) { std::sort(vec.begin(), vec.end()); }

void sort(auto &vec, auto less) { std::sort(vec.begin(), vec.end(), less); }

int main() {
  std::vector<int> numbers{3, 1, 2};
  append(numbers, std::vector{4, 0});
  sort(numbers);
  sort(numbers, [](int a, int b) { return a > b; });
  for (int number : numbers) {
    std::cout << number << '\n';
  }
}
```

`append` 保留了显式的 `T` 来固定元素类型，`const auto &y` 再接受任意可遍历容器。两个 `sort` 各自把容器和比较器推导成独立的模板参数；调用方不需要写 `<std::vector<int>>`。

## 进阶用法

`auto` 参数可以带概念约束，把“必须是整数”或“必须是范围”写进签名。约束参与重载选择，也在不满足时给出较早的诊断。

```cpp
void advance(std::integral auto &counter) { ++counter; }

template <typename T>
void append(std::vector<T> &x, const auto &y) {
  x.insert(x.end(), y.begin(), y.end());
}

void sort(auto &vec) { std::sort(vec.begin(), vec.end()); }
void sort(auto &vec, auto less) { std::sort(vec.begin(), vec.end(), less); }
```

`advance` 只接受满足 `std::integral` 的参数；两个 `sort` 靠参数数量区分，容器和比较器都可以各自推导。mold 的 `append` 展示了混合写法：显式模板参数负责元素类型，缩写参数负责容器类型，调用点只写实际实参。

完整示例见 [`examples/cpp20/abbreviated_templates.cpp`](../../examples/cpp20/abbreviated_templates.cpp)。

## 真实项目证据

| 项目 | 位置                                                                                                                            | 用法                                    | 为什么典型                                           |
| ---- | ------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------- | ---------------------------------------------------- |
| mold | [`lib/common.h#L412-L415`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/common.h#L412-L415) | 显式元素类型 + `const auto&` 接收源容器 | 只约束必要的类型，其余交给推导，避免为每种容器写重载 |
| mold | [`lib/common.h#L430-L432`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/common.h#L430-L432) | 单个 `auto&` 参数统一排序任意容器       | 辅助函数只依赖迭代器接口，模板头没有额外信息量       |
| mold | [`lib/common.h#L434-L436`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/common.h#L434-L436) | 两个 `auto` 参数分别推导容器与比较器    | 参数各自独立推导，调用点无需显式写出比较器类型       |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在 mold 的生产公共头文件中，覆盖“混合显式参数”“单参数重载”“双参数重载”三种写法。

## 陷阱与边界

**每个 `auto` 参数各自推导。** `void f(auto a, auto b)` 接受两个不同的类型；要求同类型时写成 `template <typename T> void f(T a, T b)`，或在参数上加 `std::same_as<T>` 约束。

**不能为隐式参数显式指定类型。** `void f(auto value)` 没有名字可供书写，`f<int>(1)` 不合法；需要显式模板实参时，保留一个显式模板参数。

**花括号实参不能推导。** `f({1, 2})` 无法从初始化列表推导 `auto` 的类型；先构造具名容器，或增加 `std::initializer_list` 重载。

**按值 `auto` 会丢弃引用和顶层 cv。** `void f(auto value)` 复制实参；需要修改或避免拷贝时写 `auto&`、`const auto&` 或 `auto&&`。

**缩写模板仍是函数模板。** 它不能是虚函数，定义通常要放在头文件中；与显式模板版本同时存在时，两者是不同候选，调用可能产生歧义。

**约束不是自动的。** 不写概念的 `auto` 等价于无约束模板参数；需要类型能力时显式加概念，否则错误会在函数体实例化时才出现。

**没有独立的特性测试宏。** 标准未为本特性单列宏，通常结合 `__cpp_concepts >= 202002L` 与编译器版本判断，不能只看 `__cplusplus`。

## 库联动

- **mold**：`lib/common.h` 用缩写模板实现 `append` 与两个 `sort` 重载，调用方直接传容器和比较器。
- **与泛型 lambda 的分工**：泛型 lambda 的 `auto` 参数只服务于该 lambda；缩写函数模板生成的是可重用的命名函数，能参与普通重载与 ADL。
- **与显式模板的分工**：需要显式指定类型、做部分特化或写复杂 `requires` 子句时，完整模板头更清楚；缩写形式适合参数类型只出现一次的短辅助函数。

## 横向对比

| 方案         | 是否需要模板头 | 可显式指定类型 | 约束写法        | 主要代价                                   |
| ------------ | -------------- | -------------- | --------------- | ------------------------------------------ |
| 显式函数模板 | 是             | 是             | `requires` 子句 | 模板参数只出现一次时仍要命名               |
| 缩写函数模板 | 否             | 否             | 概念 + `auto`   | C++20 起可用；隐式参数无法显式指定         |
| 泛型 lambda  | 否             | 否             | 概念 + `auto`   | 只在该 lambda 内有效，不能作为独立函数复用 |
| 普通重载     | 否             | 不适用         | 无              | 每种类型重复实现，维护成本高               |

跨标准演进：C++20 通过 P1141R2 引入缩写函数模板，与 concepts 一同落地；C++23 没有改变推导规则，仍不能为隐式引入的模板参数显式指定实参。
