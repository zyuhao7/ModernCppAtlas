---
id: cpp20.remove-cvref-type-identity
title: std::remove_cvref 与 std::type_identity
standard: C++20
kind: library
category: [元编程, 类型工具]
difficulty: 2
status: final
tier: A
compilers: {gcc: "9", clang: "8", msvc: "19.21"}
tags: [type_traits, remove_cvref, type_identity, 转发, 模板推导]
alternatives: [std::decay_t, remove_cv_t 与 remove_reference_t 组合, 自定义 trait]
proposal: P0550R2
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "特性可用时把 remove_cvref 别名到标准实现"
    evidence: [ev-abseil-remove-cvref-alias-001]
  - id: c2
    statement: "特性可用时把 type_identity 别名到标准实现"
    evidence: [ev-abseil-type-identity-alias-002]
  - id: c3
    statement: "用 std::remove_cvref_t 把成员表达式还原成裸 tuple 类型"
    evidence: [ev-drogon-remove-cvref-coroutine-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

转发引用会把实参类型推导成 `U&`、`const U&` 等形式。想把“去掉引用和顶层 cv 后的类型”存下来，过去要写 `std::remove_cv_t<std::remove_reference_t<T>>`；用 `std::decay_t` 又会顺带把数组和函数退化为指针，语义过重。

`std::type_identity` 解决的是另一个问题：模板推导时，某些位置需要“不推导”的上下文。它把类型原样包一层，让编译器停止从实参推导，常用于元编程、检测惯用法和重载消歧。

C++20 把这两个工具收进标准库：`std::remove_cvref_t<T>` 精确去掉引用与顶层 cv，`std::type_identity_t<T>` 提供非推导的类型恒等包装。

## 基础用法

<!-- compile -->

```cpp
#include <iostream>
#include <type_traits>

template <typename T> void show() {
  static_assert(std::is_same_v<std::remove_cvref_t<T>, int>);
}

int main() {
  show<const int &>();
  std::type_identity_t<int> value = 3;
  std::cout << value << '\n';
}
```

`std::remove_cvref_t<const int&>` 是 `int`；`std::type_identity_t<int>` 仍然是 `int`，但它的作用是让类型处于非推导上下文，而不是改变类型本身。

## 进阶用法

转发函数把实参存进成员时，`T&&` 推导出的 `T` 可能带引用；先用 `remove_cvref_t` 得到裸值类型，再决定成员如何声明，可以避免把引用意外存进对象。

```cpp
template <typename T> struct Holder {
  using value_type = std::remove_cvref_t<T>;
  T value;
};
template <typename T> Holder(T &&) -> Holder<T>;

template <typename T>
void select(T value, std::type_identity_t<T> same) {
  std::cout << value + same << '\n';
}
```

`Holder` 的推导指引保留原始 `T`，成员类型由调用方决定；`value_type` 则明确剥掉引用和顶层 cv，供算法或接口使用。`select(1, 2)` 中 `T` 从第一个参数推导为 `int`，第二个参数的 `type_identity_t<T>` 不参与推导，只做类型匹配；如果实参类型不同，编译失败而不是悄悄推导出另一套类型。

与 `decay_t` 的区别仍在数组和函数：`remove_cvref_t<const int(&)[3]>` 保持 `int[3]`，`decay_t` 会退化成 `int*`。需要保留数组边界时不能图省事。

完整示例见 [`examples/cpp20/remove_cvref_type_identity.cpp`](../../examples/cpp20/remove_cvref_type_identity.cpp)。

## 真实项目证据

| 项目       | 位置                                                                                                                                                                               | 用法                                         | 为什么典型                                                       |
| ---------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------- | ---------------------------------------------------------------- |
| abseil-cpp | [`absl/meta/type_traits.h#L235-L240`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/meta/type_traits.h#L235-L240)                        | 特性可用时把 `remove_cvref` 别名到标准实现   | 兼容层保留旧名称，让 C++20 编译器使用标准库实现                  |
| abseil-cpp | [`absl/meta/type_traits.h#L255-L257`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/meta/type_traits.h#L255-L257)                        | 特性可用时把 `type_identity` 别名到标准实现  | 同一个头文件统一新旧类型工具，调用方不必区分标准版本             |
| Drogon     | [`lib/inc/drogon/utils/coroutine.h#L858-L860`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/utils/coroutine.h#L858-L860) | 用 `std::remove_cvref_t` 还原成员 tuple 类型 | `decltype(member)` 带引用，剥掉后才能用 `tuple_element_t` 取元素 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。前两条由特性测试宏保护，第三条是协程等待器中的实际类型计算。

## 陷阱与边界

**`remove_cvref` 不做数组和函数退化。** `std::remove_cvref_t<const int(&)[3]>` 是 `int[3]`，不是 `int*`；需要退化时用 `std::decay_t`。两者语义不同，不能互相替代。

**只去掉顶层 cv。** `const int*` 经过 `remove_cvref` 仍然是 `const int*`；它不会移除指针指向对象的 const。需要移除更深层的 cv 时，写自定义 trait 或逐层处理。

**`type_identity` 不改变类型。** 它的价值在推导语境，而不是类型变换。把 `type_identity_t<int>` 当成“某种 int 包装”会导致误用。

**非推导上下文有明确边界。** `type_identity_t<T>` 可以阻止从该参数推导 `T`，但不会阻止其他参数推导；重载设计仍要检查整体推导结果。

**不要用 `decay_t` 图省事。** `decay_t` 会把数组和函数退化为指针，还会去掉引用与 cv；在只想剥掉引用和顶层 cv 的场景，它会悄悄改变语义。

**按值存储时仍要转发。** `remove_cvref_t<T>` 只是类型计算；把参数存入容器或回调时，仍要用 `std::forward` 保留调用方的值类别，否则会多一次拷贝。

**特性支持要检测。** `__cpp_lib_remove_cvref` 与 `__cpp_lib_type_identity` 分别对应两个设施；旧标准库可能只提供其中一个。

## 库联动

- **Abseil**：`type_traits.h` 在特性宏下把 `remove_cvref` 与 `type_identity` 别名到标准实现，旧标准库则保留自定义 trait。
- **Drogon**：协程等待器用 `std::remove_cvref_t` 处理成员 tuple 类型，再交给 `std::tuple_element_t` 计算每个任务的结果类型。
- **与 `std::forward_like` 的分工**：C++23 的 `forward_like` 在转发成员或嵌套对象时保持值类别；`remove_cvref` 只做类型剥离，不负责转发表达式。

## 横向对比

| 方案                                 | 去掉引用 | 去掉顶层 cv | 数组/函数退化 | 主要代价                         |
| ------------------------------------ | -------- | ----------- | ------------- | -------------------------------- |
| `std::remove_cvref_t`                | 是       | 是          | 否            | C++20 起可用；只处理顶层 cv      |
| `std::decay_t`                       | 是       | 是          | 是            | 语义更重，可能把数组退化为指针   |
| `remove_cv_t<remove_reference_t<T>>` | 是       | 是          | 否            | 写法冗长，C++20 前常见的替代     |
| `std::type_identity_t`               | 否       | 否          | 否            | 不改变类型，只影响模板推导       |
| 自定义 trait                         | 按需     | 按需        | 按需          | 需要维护，容易与标准行为产生偏差 |

跨标准演进：C++20 通过 P0550R2 引入 `std::remove_cvref`，通过 P0887R1 引入 `std::type_identity`。C++23 的 `std::forward_like` 继续完善转发场景，但它解决的是“按对象值类别转发成员”的问题，不替代这两个基础类型工具。
