---
id: cpp20.consteval-constinit
title: consteval 与 constinit
standard: C++20
kind: both
category: [编译期, 初始化]
difficulty: 3
status: final
tier: B
compilers: {gcc: "11", clang: "17", msvc: "19.29"}
tags: [consteval, constinit, 立即函数, 静态初始化, 常量求值]
alternatives: [constexpr, static 局部变量, 显式初始化函数]
proposal: P1073R3
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "用 consteval 在编译期计算目标平台的分支距离上限"
    evidence: [ev-mold-consteval-max-distance-001]
  - id: c2
    statement: "用特性测试宏把兼容宏映射到标准 constinit，保证静态存储期变量常量初始化"
    evidence: [ev-abseil-constinit-macro-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

`constexpr` 函数有两种执行方式：实参是常量表达式时可以在编译期求值，否则在运行期执行。库作者想强制某个函数只在编译期运行，过去只能靠约定、`static_assert` 技巧或编译器扩展。

静态存储期变量还有另一类问题：动态初始化发生在 `main` 之前，跨翻译单元的顺序未指定，容易触发“静态初始化顺序灾难”。`constexpr` 变量能保证常量初始化，但也强制变量为 `const`，无法表达“初始化必须是常量，之后可以修改”。

C++20 通过 P1073R3 引入 `consteval`，通过 P1143R2 引入 `constinit`。`consteval` 声明立即函数，所有调用都必须在常量求值中完成；`constinit` 要求静态或线程存储期变量使用常量初始化，但不要求变量本身是 `const`。

## 基础用法

<!-- compile -->

```cpp
#include <iostream>

consteval int square(int value) { return value * value; }

constinit int global_counter = square(4);

int main() {
  global_counter += 1;
  std::cout << global_counter << ' ' << square(5) << '\n';
}
```

`square` 只能以常量表达式调用；把运行期变量传进去会直接编译失败。`global_counter` 由 `square(4)` 常量初始化，因此在进入 `main` 前已经确定，不存在动态初始化顺序问题；它仍然可以在运行时递增。

## 真实项目证据

| 项目        | 位置                                                                                                                                                      | 用法                                              | 为什么典型                                                   |
| ----------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------- | ------------------------------------------------------------ |
| rui314/mold | [`src/thunks.cc#L36-L43`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/thunks.cc#L36-L43)                             | 用 `consteval` 计算目标平台的分支距离上限         | 目标架构在编译期已知，结果参与代码生成，不应在运行时重复计算 |
| abseil-cpp  | [`absl/base/attributes.h#L842-L847`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/base/attributes.h#L842-L847) | 用 `__cpp_constinit` 检测并映射到标准 `constinit` | 兼容层在可用时直接使用标准关键字，明确静态初始化保证         |

两条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。前者是 `consteval` 的生产用例，后者展示 `constinit` 的跨标准落地方式。

## 陷阱与边界

**`consteval` 不能接收运行期输入。** `std::cin >> n; square(n);` 直接编译失败，即使该分支在运行时不会执行也一样。需要运行期计算时用 `constexpr` 函数，而不是把 `consteval` 当作普通函数。

**立即函数不能延迟到运行期调用。** 取 `consteval` 函数地址并保存为函数指针通常直接被拒绝；需要运行期多态时改用普通函数或 `constexpr` 函数。

**`constinit` 只适用于静态或线程存储期变量。** 函数内的自动变量不能标 `constinit`；局部变量需要常量初始化时直接用 `constexpr` 或普通初始化。

**`constinit` 不保证变量不可变。** 它只约束初始化方式；变量之后仍可被修改，多线程写入仍需同步。把 `constinit` 当成 `const` 会引入数据竞争。

**初始化表达式必须是常量表达式。** 依赖运行期配置、堆分配或非 `constexpr` 构造函数的初始值不能用 `constinit`；编译器的诊断会指出哪一部分不是常量。

**`constexpr` 变量已经是常量初始化。** 对 `constexpr` 变量不能再叠加 `constinit`，标准规定两者最多只能出现一个；需要可变全局量时才用 `constinit`。

**静态初始化顺序问题没有完全消失。** `constinit` 只保证该变量自身没有动态初始化；其他仍走动态初始化的全局对象依旧有顺序问题，跨翻译单元的依赖要显式设计。

**编译器支持要用特性测试宏判断。** 使用 `__cpp_consteval` 与 `__cpp_constinit` 做条件编译；不同编译器对 `consteval` 的完整支持版本差异较大，不能只按语言标准开关判断。

## 横向对比

| 写法                  | 求值时机         | 变量可变 | 常量初始化保证 | 主要用途                           |
| --------------------- | ---------------- | -------- | -------------- | ---------------------------------- |
| `constexpr` 函数      | 编译期或运行期   | —        | —              | 两种场景复用同一实现               |
| `consteval` 函数      | 仅编译期         | —        | —              | 强制立即求值的编译期计算           |
| `constinit` 变量      | 初始化必须编译期 | 是       | 是             | 避免静态初始化顺序问题，保留可变性 |
| `constexpr` 变量      | 编译期           | 否       | 是             | 编译期常量                         |
| 函数内 `static`       | 首次调用时       | 是       | 否             | 惰性初始化；C++11 起线程安全       |
| 显式初始化函数 + 断言 | 运行期           | 是       | 否             | 需要运行期依赖的全局状态           |

跨标准演进：C++11 引入 `constexpr`；C++20 增加 `consteval` 与 `constinit`；C++23 扩展 `constexpr` 对部分标准库类型的支持，但不会改变 `consteval` 的强制编译期语义。`constinit` 与 `consteval` 解决的是不同问题：一个约束变量初始化，一个约束函数调用时机。
