---
id: cpp14.variable-templates
title: 变量模板
standard: C++14
kind: language
category: [模板, 编译期计算]
difficulty: 2
status: final
tier: A
compilers: {gcc: "5", clang: "3.4", msvc: "19.0"}
tags: [variable template, 变量模板, constexpr, 类型特征, _v 后缀]
alternatives: [类模板静态成员, constexpr 函数, 宏, 类型特征 value]
proposal: N3651
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用变量模板为可等待类型提供 _v 后缀布尔常量"
    evidence: [ev-drogon-variable-template-awaitable-001]
  - id: c2
    statement: "用变量模板主模板加偏特化识别字符串类型"
    evidence: [ev-abseil-variable-template-string-002]
  - id: c3
    statement: "用变量模板封装类型特征组合后的布尔结果"
    evidence: [ev-abseil-variable-template-result-match-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++14 之前，模板化的编译期常量只能写成类模板的静态成员：`std::is_integral<T>::value`、`std::tuple_size<T>::value`。这种写法冗长，而且每引入一个特征就要定义一个结构体。宏可以给出短名字，但没有类型、没有作用域，也无法参与模板特化。函数模板能返回值，却不能直接作为模板实参或数组维度。

变量模板把“模板参数化的变量”变成语言特性：一次声明即可针对每个类型生成一个常量，使用时写 `name<T>`，像普通变量一样参与表达式。标准库从 C++17 起大规模采用 `_v` 后缀命名，例如 `std::is_integral_v<T>`，其底层机制正是 C++14 的变量模板。标准提案为 N3651。

## 基础用法

在变量声明前加 `template` 和模板参数列表。变量可以是 `constexpr`，也可以是普通对象。

<!-- compile -->

```cpp
#include <iostream>

template <typename T>
constexpr T pi = T(3.1415926535897932385);

template <typename T>
constexpr bool is_small = sizeof(T) <= sizeof(int);

int main() {
  static_assert(is_small<char>, "char fits in int");
  static_assert(!is_small<double>, "double is wider than int");

  std::cout << pi<double> << ' ' << pi<float> << '\n';
}
```

输出 `3.14159 3.14159`。`pi<double>` 与 `pi<float>` 是两个独立实例，各自按目标类型完成舍入；`is_small<T>` 直接作为 `static_assert` 的条件。完整版本见 [`examples/cpp14/variable_templates.cpp`](../../examples/cpp14/variable_templates.cpp)。

## 进阶用法

变量模板支持偏特化和显式特化，因此可以像类模板一样按类型分派。abseil 的字符串识别就是一个例子：主模板给 `false`，再对 `std::basic_string` 偏特化为 `true`。与类模板静态成员相比，调用处少一层 `::value`，可读性明显更好。

变量模板不必是 `constexpr`。它可以是任意类型的变量，编译器为每个实参生成独立实例；但只有 `constexpr` 实例才能用于模板实参、数组维度和 `static_assert`。标准库的 `_v` 特征都定义为 `inline constexpr`，既保证编译期可用，也避免头文件中的重复定义。

C++17 的 `inline` 变量解决了链接问题：C++14 中，头文件里的 `constexpr` 变量模板在每个翻译单元各有一份，取地址会得到不同实体；加 `inline` 后整个程序共享同一实体。跨翻译单元比较地址或注册回调时要注意这个差异。

需要“函数式”的编译期计算时，变量模板不能替代 `constexpr` 函数：它没有参数，只能按类型或非类型模板参数实例化。复杂计算仍应写成 `constexpr` 函数，再把结果存进变量模板。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                                      | 用法                                 | 为什么典型                             |
| ------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------ | -------------------------------------- |
| drogon | [`lib/inc/drogon/utils/coroutine.h#L87-L88`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/utils/coroutine.h#L87-L88)                            | 用 `_v` 变量模板包装可等待类型判断   | 调用处直接写布尔常量，无需 `::value`   |
| abseil | [`absl/strings/internal/generic_printer_internal.h#L73-L77`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/strings/internal/generic_printer_internal.h#L73-L77) | 变量模板主模板加字符串偏特化         | 布尔特征保持变量语法，特化表达类型范围 |
| abseil | [`absl/status/status_builder.h#L75-L77`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/status/status_builder.h#L75-L77)                                         | 把多层类型特征组合收敛为一个布尔常量 | 后续 `enable_if` 约束因此更易读        |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖布尔特征、偏特化与约束条件三种用途。

## 陷阱与边界

**需要 C++14。** C++11 只有类模板静态成员；跨旧工具链时可用 `struct is_x { static constexpr bool value = ...; };` 模拟，调用处保留 `::value`。

**使用时要写出模板实参。** 没有函数实参可供推导，`pi` 单独出现无法确定类型，必须写 `pi<double>`；C++17 的类模板实参推导不适用于变量模板。

**链接与地址。** C++14 头文件中的 `constexpr` 变量模板可能每个翻译单元各有一份；需要唯一实体时用 C++17 的 `inline constexpr`，或把定义放到单一源文件。

**显式特化要先声明。** 特化必须出现在首次使用之前，否则编译器可能已经实例化主模板，导致 ODR 违规；头文件中通常把特化紧跟主模板定义。

**不是所有类型都能 `constexpr`。** 非字面量类型、含动态初始化或非平凡析构的类型不能作为 `constexpr` 变量模板，只能作为普通变量模板使用。

**别用它替代函数。** 变量模板没有运行期参数，不能表达依赖输入的计算；把复杂逻辑硬塞进模板参数会让编译错误难以阅读。

## 库联动

- **`<type_traits>`**：C++17 起所有 `_v` 后缀特征（`std::is_integral_v`、`std::is_same_v` 等）都是变量模板。
- **`<tuple>` / `<array>`**：`std::tuple_size_v`、`std::tuple_element_t` 等提供更短的编译期查询。
- **模板约束**：`enable_if_t`、`void_t` 与 `_v` 特征组合，是 concepts 之前最常见的 SFINAE 写法。
- **abseil / drogon**：基础库与协程工具用变量模板把类型判断收敛成可读的布尔常量。

## 横向对比

| 方案                 | 使用语法          | 可特化 | 编译期常量     | 主要限制                   |
| -------------------- | ----------------- | ------ | -------------- | -------------------------- |
| 变量模板             | `trait<T>`        | 是     | 需 `constexpr` | C++14 起可用，需写模板实参 |
| 类模板静态成员       | `trait<T>::value` | 是     | 是             | 样板代码多，语法冗长       |
| `constexpr` 函数模板 | `trait<T>()`      | 重载   | 是             | 调用语法多一对括号         |
| 宏                   | `TRAIT(T)`        | 否     | 是             | 无类型、无作用域、难调试   |

跨标准演进：C++14 通过 N3651 引入变量模板；C++17 增加 `inline` 变量并让标准库全面采用 `_v` 后缀；C++20 的 concepts 提供更直接的约束语法，但变量模板仍是表达类型常量的常用工具。
