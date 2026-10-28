---
id: cpp11.auto-decltype
title: auto 与 decltype
standard: C++11
kind: language
category: [类型系统, 语法]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.8.1", clang: "2.9", msvc: "17.0"}
tags: [auto, decltype, 类型推导, 尾置返回类型, 迭代器]
alternatives: [显式类型, typedef, using 别名, 模板参数推导]
proposal: N1984
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用 auto 承接 upper_bound 返回的迭代器类型"
    evidence: [ev-mold-auto-upper-bound-001]
  - id: c2
    statement: "用 decltype 复用结构体成员的声明类型定义别名"
    evidence: [ev-mold-decltype-offset-002]
  - id: c3
    statement: "用 decltype 查询 awaiter 的 await_resume 返回类型"
    evidence: [ev-drogon-decltype-await-result-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

泛型代码里的类型常常又长又依赖上下文。`std::map<std::string, std::vector<int>>::const_iterator` 这样的类型写一次还好，写在循环、lambda 和模板里就会淹没逻辑。更麻烦的是，有些类型根本无法直接命名：lambda 的闭包类型、依赖模板参数的表达式结果、`decltype` 才能描述的成员类型。

C++11 引入 `auto` 和 `decltype` 解决两类问题。`auto` 让编译器从初始化器推导变量类型，调用点只写 `auto`；`decltype(expr)` 在编译期给出表达式的精确类型，保留引用和 cv 限定，常用于尾置返回类型和类型别名。标准提案为 N1984（`auto`）与 N2343/N3276（`decltype`）。

## 基础用法

`auto` 必须有初始化器；`decltype` 接受任意表达式。C++11 的函数返回类型若需推导，要写成尾置返回类型。

<!-- compile -->

```cpp
#include <cstddef>
#include <iostream>
#include <vector>

template <typename Container>
auto middle(const Container& values) -> decltype(values.begin()) {
  return values.begin() + static_cast<std::ptrdiff_t>(values.size() / 2);
}

int main() {
  std::vector<int> values{10, 20, 30, 40, 50};
  auto it = middle(values);
  decltype(values.size()) count = values.size();
  std::cout << *it << ' ' << count << '\n';
}
```

输出 `30 5`。`auto it` 的迭代器类型由 `middle` 的返回类型决定，`decltype(values.size())` 精确取得 `size()` 的返回类型。完整版本见 [`examples/cpp11/auto_decltype.cpp`](../../examples/cpp11/auto_decltype.cpp)。

## 进阶用法

`auto` 的推导规则与模板参数推导一致：顶层 `const` 和引用被丢弃，数组和函数退化为指针。要保留引用需写 `auto&` 或 `const auto&`；要保留 `const` 需写 `const auto`。这让 `auto` 适合承接迭代器、函数返回值与局部副本，但不适合无条件替代所有显式类型。

`decltype` 的规则不同：`decltype(x)` 给出变量声明时的类型，`decltype((x))` 对带括号的左值表达式给出引用类型。这个差异在泛型代码里很关键，也是 C++14 `decltype(auto)` 存在的原因。C++14 还允许普通函数用 `auto` 推导返回类型，不再需要尾置写法；C++11 中 `auto f()` 没有尾置返回类型是非法的。

尾置返回类型让返回类型可以引用参数：`template <class T, class U> auto add(T a, U b) -> decltype(a + b)`。C++14 的 `decltype(auto)` 则把推导规则交给 `decltype`，适合完美转发包装器。C++20 的简写函数模板进一步允许 `auto` 参数，但返回类型推导规则不变。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                           | 用法                                   | 为什么典型                                 |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | -------------------------------------- | ------------------------------------------ |
| mold   | [`src/mold.h#L2624-L2630`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/mold.h#L2624-L2630)                                                | 用 `auto` 承接 `upper_bound` 的迭代器  | 迭代器类型依赖容器与模板参数，写出来冗长   |
| mold   | [`src/gdb-index.cc#L406-L411`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/gdb-index.cc#L406-L411)                                        | 用 `decltype` 复用字段类型定义偏移别名 | 别名与字段声明自动同步，避免重复类型       |
| drogon | [`lib/inc/drogon/utils/coroutine.h#L64-L69`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/utils/coroutine.h#L64-L69) | 用 `decltype` 查询 `await_resume` 类型 | 协程结果类型无法直接命名，只能由表达式推导 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖迭代器推导、类型别名与依赖表达式三种用途。

## 陷阱与边界

**`auto` 会丢弃引用和顶层 `const`。** 从函数返回引用时，`auto x = f()` 得到的是副本；要绑定引用必须写 `auto&` 或 `const auto&`。

**代理对象不是值。** `std::vector<bool>` 的 `operator[]` 返回代理引用，`auto bit = v[0]` 得到代理对象而非 `bool`；需要真实值时要显式写 `bool` 或 `static_cast`。

**`decltype` 的括号陷阱。** `decltype(x)` 是变量类型，`decltype((x))` 是左值引用类型；宏和模板里多一层括号可能改变结果。

**尾置返回类型不能省。** C++11 中 `auto f() { return 1; }` 非法，必须写 `auto f() -> int`；C++14 才允许省略。

**不要用 `auto` 隐藏转换。** 需要显式窄化、接口类型或文档作用时，写清楚类型；`auto` 适合局部实现细节，不适合公共接口的契约表达。

**函数参数不能用 `auto`。** C++11/14 的普通函数参数不能写 `auto`；C++14 仅泛型 lambda 允许，C++20 才支持简写函数模板。

## 库联动

- **标准容器与算法**：迭代器类型几乎总是用 `auto` 承接，`begin()`、`find()`、`upper_bound()` 的返回值无需拼写。
- **`<type_traits>`**：`decltype` 与 `std::declval` 组合，是 SFINAE 和返回类型推导的基础工具。
- **智能指针与 RAII**：`auto ptr = std::make_unique<T>()` 避免重复写 `std::unique_ptr<T>`。
- **mold / drogon**：链接器用 `auto` 简化迭代器，协程工具用 `decltype` 查询无法命名的结果类型。

## 横向对比

| 方案             | 保留引用 | 保留 `const` | 可用于返回类型 | 主要限制                   |
| ---------------- | -------- | ------------ | -------------- | -------------------------- |
| 显式类型         | 是       | 是           | 是             | 冗长，依赖上下文时难以书写 |
| `auto`           | 否       | 否           | C++14 起可     | 丢弃引用与顶层 `const`     |
| `decltype(expr)` | 是       | 是           | 是             | 括号会改变左值/引用结果    |
| `decltype(auto)` | 是       | 是           | C++14 起可     | 推导规则隐式，阅读成本较高 |

跨标准演进：C++11 通过 N1984 引入 `auto`、通过 N2343/N3276 完善 `decltype`；C++14 增加返回类型推导、`decltype(auto)` 与泛型 lambda；C++20 允许简写函数模板参数。
