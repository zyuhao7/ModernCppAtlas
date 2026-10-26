---
id: cpp17.invoke-apply
title: std::invoke 与 std::apply
standard: C++17
kind: library
category: [泛型, 可调用对象]
difficulty: 3
status: final
tier: B
compilers: {gcc: "7.1", clang: "16", msvc: "19.13"}
tags: [invoke, apply, 成员指针, tuple, 完美转发]
alternatives: [直接调用, std::bind, lambda, std::mem_fn]
proposal: N4169
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "用 std::apply 把捕获的 tuple 展开成 SQL 参数绑定调用"
    evidence: [ev-drogon-apply-sql-binder-001]
  - id: c2
    statement: "用 std::invoke 在 call_once 路径上调用任意可调用对象"
    evidence: [ev-abseil-invoke-once-callable-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

泛型代码拿到一个可调用对象时，不知道它是函数指针、仿函数、成员函数指针还是 `reference_wrapper`。直接写 `f(args...)` 覆盖不了成员指针，`std::bind` 又会引入占位符、嵌套绑定和转发规则。拿到一个 tuple 时，想调用 `f(a, b, c)`，还得手写下标和 `std::get`。

`std::invoke` 提供统一的调用语法，按标准 `INVOKE` 规则处理成员指针与引用包装；`std::apply` 把 tuple-like 对象的元素展开成实参。C++17 通过 N4169 引入 `std::invoke`，通过 P0220R1 引入 `std::apply`。两者把“怎么调用”从调用点抽出来，适合转发层、回调和类型擦除实现。

## 基础用法

<!-- compile -->

```cpp
#include <functional>
#include <iostream>
#include <tuple>

struct Counter {
  int value;
  int get() const { return value; }
};

int main() {
  Counter counter{7};
  std::cout << std::invoke(&Counter::get, counter) << '\n';

  auto args = std::make_tuple(2, 3);
  std::cout << std::apply([](int a, int b) { return a * b; }, args) << '\n';
}
```

`std::invoke(&Counter::get, counter)` 等价于 `counter.get()`；成员指针、对象引用和参数由 `invoke` 统一拼装。`std::apply` 把 `args` 的两个元素依次传给 lambda，返回 `6`。

## 真实项目证据

| 项目       | 位置                                                                                                                                                                                 | 用法                                             | 为什么典型                                                 |
| ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------ | ---------------------------------------------------------- |
| Drogon     | [`orm_lib/inc/drogon/orm/Criteria.h#L119-L128`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/inc/drogon/orm/Criteria.h#L119-L128) | 用 `std::apply` 展开捕获的 tuple 并绑定 SQL 参数 | 参数个数在构造时确定，展开逻辑集中在一处，避免手写下标     |
| abseil-cpp | [`absl/base/call_once.h#L180-L190`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/base/call_once.h#L180-L190)                              | 用 `std::invoke` 调用任意 Callable 并转发参数    | 同步原语必须支持函数指针、仿函数和成员指针，同时保留值类别 |

两条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。前者展示 tuple 展开，后者展示通用调用。

## 陷阱与边界

**`std::apply` 只接受 tuple-like 类型。** `std::tuple`、`std::pair`、`std::array` 可以直接用；自定义类型需要提供 `std::tuple_size`、`std::tuple_element` 和 `get`。普通聚合体不能直接展开。

**值类别由 tuple 决定。** `std::apply(f, tuple)` 把元素作为左值传入；`std::apply(f, std::move(tuple))` 把元素作为右值传入。转发层要根据调用方是否还持有 tuple 选择，移动后不要再次读取。

**不要依赖实参求值顺序。** 标准不保证展开后的实参按从左到右求值；带有相互依赖副作用的调用应拆开，或先算出结果再展开。

**成员指针需要正确对象形式。** `std::invoke` 接受对象、对象指针或 `reference_wrapper`；数据成员指针返回对应成员的引用，注意对象生命周期。成员函数重载需要先 `static_cast` 到明确签名，`invoke` 无法替你选择重载。

**`std::bind` 不是 `std::invoke` 的替代品。** `bind` 适合预先绑定部分参数，但嵌套绑定、占位符和转发规则容易出错；泛型转发通常用 lambda 或直接 `invoke` 更清楚。

**异常会原样传播。** `invoke` 和 `apply` 不捕获异常，也不改变调用约定；回调边界需要自己处理错误与清理。

**C++20 起可用于常量求值。** `std::invoke` 与 `std::apply` 在 C++20 成为 `constexpr`，可以在编译期展开 tuple；C++17 代码不能依赖这一点。

## 横向对比

| 方案              | 成员指针      | 转发值类别 | tuple 展开 | 主要代价                              |
| ----------------- | ------------- | ---------- | ---------- | ------------------------------------- |
| 直接调用 `f(...)` | 否            | 由写法决定 | 否         | 泛型代码无法统一处理成员指针          |
| `std::invoke`     | 是            | 是         | 否         | 需要 `<functional>`；重载仍需显式选择 |
| `std::apply`      | 通过 `invoke` | 是         | 是         | 只支持 tuple-like；求值顺序不保证     |
| `std::bind`       | 部分          | 规则复杂   | 否         | 占位符与嵌套绑定可读性差              |
| lambda            | 可手工处理    | 显式       | 否         | 需要为每种签名单独编写                |
| `std::mem_fn`     | 是            | 是         | 否         | 只处理成员指针，不覆盖普通可调用对象  |

跨标准演进：C++17 引入 `std::invoke` 与 `std::apply`；C++20 使两者可用于常量求值；C++23 增加 `std::invoke_r`，允许显式指定返回类型并处理可转换的返回值。
