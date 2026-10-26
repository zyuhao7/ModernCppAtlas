---
id: cpp17.constexpr-lambda
title: constexpr lambda
standard: C++17
kind: language
category: [lambda, 编译期]
difficulty: 2
status: final
tier: A
compilers: {gcc: "7", clang: "5", msvc: "19.11"}
tags: [constexpr, lambda, 编译期, 常量求值, 查表]
alternatives: [普通 constexpr 函数, 手写常量表, 立即调用函数, 宏生成]
proposal: P0170R1
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "用立即调用的 constexpr lambda 在编译期生成转义序列表"
    evidence: [ev-abseil-constexpr-lambda-escape-001]
  - id: c2
    statement: "用 constexpr lambda 在编译期填充并返回数组，再用 static_assert 校验"
    evidence: [ev-abseil-constexpr-lambda-for-each-002]
  - id: c3
    statement: "用 constexpr lambda 在编译期复制静态常量数组"
    evidence: [ev-abseil-constexpr-lambda-copy-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

需要一张由循环或条件生成的常量表时，过去只能写普通 `constexpr` 函数再调用，或者手工展开每一项。Lambda 适合写一次性的初始化逻辑，但 C++14 的 lambda 不能出现在常量表达式中，`constexpr` 变量无法用它初始化。

C++17 通过 P0170R1 让 lambda 在满足条件时隐式成为 `constexpr`。闭包的 `operator()` 可以像普通 `constexpr` 函数一样在编译期求值，因此“定义 lambda 并立即调用”成为生成常量表的常用写法：

```cpp
constexpr auto table = [] {
  std::array<int, 4> values{};
  for (int i = 0; i < 4; ++i) {
    values[i] = i * i;
  }
  return values;
}();
```

它不是新语法糖，而是把 lambda 的调用运算符纳入常量求值规则；能否 `constexpr` 仍取决于函数体是否满足要求。

## 基础用法

<!-- compile -->

```cpp
#include <array>
#include <iostream>

int main() {
  constexpr auto squares = [] {
    std::array<int, 4> values{};
    for (int i = 0; i < 4; ++i) {
      values[i] = i * i;
    }
    return values;
  }();

  static_assert(squares[3] == 9);
  std::cout << squares[0] << ' ' << squares[3] << '\n';
}
```

`constexpr auto squares = ...` 要求初始化表达式是常量表达式；立即调用 lambda 在编译期执行循环并返回数组。`static_assert` 进一步证明结果可用于常量求值，而不是碰巧在运行期算出来。

## 进阶用法

Lambda 可以捕获常量并在常量表达式中使用；捕获本身不阻止 `constexpr`，关键是调用时所有输入都能在编译期确定。

```cpp
constexpr int f() {
  constexpr int scale = 3;
  constexpr auto triple = [scale](int value) { return value * scale; };
  static_assert(triple(4) == 12);
  return triple(2);
}
```

`scale` 具有自动存储期且是常量表达式，按值捕获后可在 `triple` 的常量调用中使用。捕获运行期变量后，同一个 lambda 仍可用于运行期，但不能再参与常量求值。

需要固定查表数据时，把生成逻辑放进立即调用 lambda，常量表只暴露结果，调用方看不到临时数组和循环：

```cpp
constexpr auto make_table = [](int base) {
  std::array<int, 4> values{};
  for (int i = 0; i < 4; ++i) {
    values[i] = base + i;
  }
  return values;
};
constexpr auto table = make_table(10);
static_assert(table[3] == 13);
```

完整示例见 [`examples/cpp17/constexpr_lambda.cpp`](../../examples/cpp17/constexpr_lambda.cpp)。

## 真实项目证据

| 项目       | 位置                                                                                                                                                                              | 用法                                             | 为什么典型                                                |
| ---------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ | --------------------------------------------------------- |
| abseil-cpp | [`absl/strings/escaping.cc#L392-L399`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/strings/escaping.cc#L392-L399)                     | 用立即调用的 `constexpr` lambda 生成转义序列表   | 查表数据由循环生成，比手写 256 项常量更易维护             |
| abseil-cpp | [`absl/algorithm/container_test.cc#L1744-L1750`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/algorithm/container_test.cc#L1744-L1750) | 在 `constexpr` lambda 中调用算法填充数组并做断言 | 验证 lambda 与 `constexpr` 算法可以在同一次常量求值中组合 |
| abseil-cpp | [`absl/algorithm/container_test.cc#L1923-L1930`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/algorithm/container_test.cc#L1923-L1930) | 用无捕获 lambda 在编译期复制静态常量数组         | 展示无捕获 lambda 读取静态存储期常量并返回新数组的写法    |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。第一条来自生产路径，后两条来自 Abseil 的常量求值测试；测试证据在 `is_test` 中标记，不与生产证据混为一谈。

## 陷阱与边界

**不是所有 lambda 都自动 `constexpr`。** 只有函数体满足 `constexpr` 函数要求时才隐式生效；包含 C++17 不允许的语句或调用非 `constexpr` 函数时，它仍是普通 lambda。

**显式标注不会放宽限制。** 写 `[]() constexpr { ... }` 只表达意图；函数体不满足要求会直接编译失败，而不是退化为运行期 lambda。

**常量求值需要输入也是常量。** 捕获运行期变量或传入运行期实参后，lambda 可以在运行期调用，但不能初始化 `constexpr` 变量，也不能用于 `static_assert`。

**闭包类型不是稳定接口。** 每个 lambda 表达式产生唯一类型；不要把 `decltype(lambda)` 写进跨翻译单元的公共接口，也不要把闭包类型当作可长期依赖的类型名。

**`mutable` 会改变调用运算符的 const 限定。** 需要修改捕获值的 `mutable` lambda 在常量求值中的可用性受对象 const 性影响；能用无状态 lambda 时优先保持默认。

**C++17 的函数体限制仍然适用。** 静态变量、`goto`、未初始化变量等写法会阻止 `constexpr`；C++20 放宽了部分限制，但不能把 C++20 代码直接按 C++17 规则使用。

**特性检测用 `__cpp_constexpr`。** `>= 201603L` 才包含 constexpr lambda；只看 `__cplusplus` 不能区分编译器是否完整实现。

## 库联动

- **Abseil**：字符串转义表用立即调用的 constexpr lambda 生成；常量求值测试用 lambda 包裹算法调用，再以 `static_assert` 校验。
- **标准库**：`std::array` 与 C++20 起逐步 `constexpr` 化的算法可以在 lambda 体内使用；C++17 下仍受当时算法是否为 `constexpr` 的限制。
- **与普通 `constexpr` 函数的分工**：逻辑需要复用或有名字时写函数；只用于一次性初始化常量表时，立即调用 lambda 能避免污染命名空间。

## 横向对比

| 方案                | 可编译期调用 | 局部作用域 | 捕获外部常量 | 主要代价                                      |
| ------------------- | ------------ | ---------- | ------------ | --------------------------------------------- |
| `constexpr` 函数    | 是           | 否         | 需显式参数   | 需要命名；一次性逻辑会污染作用域              |
| `constexpr` lambda  | 是           | 是         | 是           | C++17 起可用；函数体限制与 constexpr 函数相同 |
| 普通 lambda         | 否           | 是         | 是           | 不能初始化 `constexpr` 变量                   |
| 手写常量表 / 宏生成 | 是           | 视写法而定 | 否           | 难以维护，循环和条件逻辑要手工展开            |

跨标准演进：C++17 通过 P0170R1 引入 constexpr lambda；C++20 放宽 constexpr 函数体限制并增加动态分配等能力，让更多 lambda 能参与常量求值；截至 C++23，闭包类型仍不是稳定的 ABI 接口。
