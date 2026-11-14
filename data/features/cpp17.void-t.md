---
id: cpp17.void-t
title: std::void_t
standard: C++17
kind: library
category: [模板, 元编程]
difficulty: 3
status: final
tier: A
compilers: {gcc: "6.1", clang: "3.6", msvc: "19.0"}
tags: [void_t, SFINAE, 检测惯用法, 偏特化, 元编程]
alternatives: [std::enable_if, requires 表达式, detection idiom, decltype + SFINAE]
proposal: N3911
last_verified: "2026-11-14"
claims:
  - id: c1
    statement: "用 std::void_t 偏特化检测成员类型，缺失时回退到主模板的默认类型"
    evidence: [ev-abseil-void-t-extract-or-001]
  - id: c2
    statement: "用 std::void_t 包裹 getAwaiter 调用表达式检测类型是否可被 co_await"
    evidence: [ev-drogon-void-t-awaitable-002]
  - id: c3
    statement: "用 std::void_t 检测自定义 meta 的 value 调用并据返回类型选择定制路径"
    evidence: [ev-glaze-void-t-custom-meta-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

模板代码经常要回答“这个类型有没有某个成员”“这个表达式是否良构”。C++17 之前只能靠 `decltype`、`enable_if` 与一层层偏特化拼出所谓的 detection idiom，主模板、偏特化、默认参数之间要反复写 `typename = void` 或 `std::enable_if_t<...>`，样板代码比检测逻辑本身还长。

`std::void_t<Ts...>` 是一个别名模板：无论传入什么类型参数，它都展开成 `void`。单独看毫无作用，放进偏特化的模板参数里却能把“替换是否成功”折叠成一个可匹配的 `void`：表达式良构，偏特化成立；表达式不成立，SFINAE 把它剔除，回退到主模板。N3911 把这套惯用法的一个小零件标准化，头文件是 `<type_traits>`。

## 基础用法

主模板继承 `std::false_type`，偏特化用 `std::void_t` 包住待检测的成员类型；检测成功时偏特化被选中，继承 `std::true_type`。

<!-- compile -->

```cpp
#include <iostream>
#include <type_traits>
#include <vector>

template <class, class = void>
struct has_value_type : std::false_type {};

template <class T>
struct has_value_type<T, std::void_t<typename T::value_type>>
    : std::true_type {};

int main() {
  std::cout << std::boolalpha << has_value_type<std::vector<int>>::value << ' '
            << has_value_type<int>::value << '\n';
}
```

输出：

```text
true false
```

完整版本见 [`examples/cpp17/void_t.cpp`](../../examples/cpp17/void_t.cpp)，六个场景的输出为：

```text
[1] vector has value_type = true
[2] int has value_type = false
[3] string has size() = true
[4] int has size() = false
[5] vector has reserve() = true
[6] vector is iterable = true
```

场景 1、2 检测成员类型 `value_type`，一个有、一个没有。场景 3、4 把成员函数调用包进 `decltype`，检测 `size()` 是否存在。场景 5 检测带参数的 `reserve(0)`。场景 6 在同一个 `void_t` 里放入 `std::begin` 与 `std::end` 两个表达式，两个都良构时才算可迭代。

## 进阶用法

**检测成员类型。** `std::void_t<typename T::value_type>` 只关心类型是否存在。abseil 的 `ExtractOr` 用这个模式在分配器具备某特征时取出对应类型，否则退回默认类型。

**检测表达式。** 成员函数、运算符、自由函数是否可调用，都要用 `decltype` 把调用包起来：`std::void_t<decltype(std::declval<T&>().reserve(0))>`。`std::declval` 只用于未求值语境，不要求类型可构造。

**多个条件合并。** `void_t` 接受任意多个类型参数，全部替换成功才会形成 `void`。drogon 用 `std::void_t<decltype(internal::getAwaiter(std::declval<T>()))>` 判断类型能否 `co_await`；本页示例则把 `begin` 与 `end` 两个表达式一起放入。

**检测与取类型合一。** 在偏特化里可以再用 `decltype` 拿到被检测表达式的返回类型。glaze 的 `has_custom_meta_impl` 先让 `void_t` 判断 `meta<T>::value` 调用良构，再在偏特化体内保存返回类型并检查它是否为 `custom_t`。

**CWG 1558 的回溯应用。** `void_t` 早期被发现在某些实现里对未使用的模板参数不做替换，导致检测失效。CWG 1558 明确参数包中的失败也应触发 SFINAE，并回溯应用到 C++11 模式；现代标准库都已按修正后的语义实现。

## 真实项目证据

| 项目       | 位置                                                                                                                                                                           | 用法                                                                  | 为什么典型                                                   |
| ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------- | ------------------------------------------------------------ |
| abseil-cpp | [`absl/memory/memory.h#L240-L253`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/memory/memory.h#L240-L253)                          | 用 `std::void_t` 偏特化检测 `Extract<Obj>` 是否良构，缺失时给默认类型 | 成员特征检测的标准形态：主模板给默认值，偏特化接管成功情形   |
| drogon     | [`lib/inc/drogon/utils/coroutine.h#L74-L85`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/utils/coroutine.h#L74-L85) | 用 `std::void_t` 包裹 `getAwaiter` 调用表达式检测可 `co_await`        | 协程库要在编译期判断 awaiter，表达式检测直接落在偏特化条件里 |
| glaze      | [`include/glaze/core/custom_meta.hpp#L23-L32`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/core/custom_meta.hpp#L23-L32) | 用 `std::void_t` 检测 `meta<T>::value` 调用并保存返回类型             | 检测成功后还要用返回类型做进一步判断，展示检测与取类型合一   |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖成员类型、调用表达式与返回值特征三种检测形态。

## 陷阱与边界

**`void_t` 只能检测类型，不执行表达式。** 它接收的是类型参数；检测表达式必须先写成 `decltype(...)`。直接传一个值或语句不能编译。

**主模板的默认参数不能省。** 偏特化要有一个“失败时”的落点，通常是 `template <class, class = void>` 的主模板。少了默认参数，检测失败时会直接报“无匹配特化”，而不是退化成 `false_type`。

**多个条件按“全部成功”处理。** 参数包里任意一个替换失败，整个偏特化都不可行，结果是假而不是逐项报告。要区分是哪一个条件失败，需要拆成多个检测器。

**不要用它替代 `enable_if` 做重载选择。** `void_t` 适合“询问是否具备某特征”；按条件启用或禁用函数重载仍然用 `std::enable_if_t` 或 C++20 的 `requires`。

**依赖 CWG 1558 的实现质量。** 极旧的编译器可能对未使用的参数包不做替换，导致检测永远为假。GCC 6.1、Clang 3.6、MSVC 19.0 起已按修正语义实现。

**C++20 之后优先用 `requires`。** 概念与 `requires` 表达式把同样的检测写得更直接、报错更清楚。新代码若已要求 C++20，不必再手写 `void_t` 偏特化。

## 库联动

- **`std::enable_if`**：按条件启用重载；`void_t` 负责检测，`enable_if` 负责选择，两者常在同一模板里配合。
- **`std::declval`**：在未求值语境中构造任意类型的表达式，是 `void_t<decltype(...)>` 的固定搭档。
- **`std::true_type` / `std::false_type`**：检测结果的标准载体，提供 `::value` 与标签分派能力。
- **`requires` 表达式**（C++20）：检测惯用法的后继，能直接表达“表达式良构”而不需要偏特化样板。
- **`std::conjunction` / `std::disjunction`**：把多个检测结果组合成复合条件。

## 横向对比

| 方案              | 表达方式                     | 检测对象       | 报错可读性 | 主要限制                        |
| ----------------- | ---------------------------- | -------------- | ---------- | ------------------------------- |
| `std::void_t`     | 主模板 + 偏特化 + `decltype` | 类型与表达式   | 一般       | 需要样板代码，失败无细节        |
| `std::enable_if`  | 模板参数或返回类型中的条件   | 编译期布尔条件 | 一般       | 关注启用/禁用，不直接检测表达式 |
| `requires` 表达式 | 内建语法                     | 类型与表达式   | 好         | C++20 起                        |
| detection idiom   | `void_t` 的完整封装          | 类型与表达式   | 一般       | 模板层数更多                    |

`std::void_t` 由 N3911 进入 C++17，用一个“永远展开成 `void`”的别名把 SFINAE 检测从繁琐的 `enable_if` 样板里解放出来。它本身不做任何计算，价值全在偏特化的替换规则上；理解这一点，才能判断何时该用它、何时该转向 C++20 的 `requires`。
