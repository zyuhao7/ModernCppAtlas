---
id: cpp20.is-constant-evaluated
title: std::is_constant_evaluated
standard: C++20
kind: library
category: [编译期计算, 语言机制]
difficulty: 3
status: final
tier: A
compilers: {gcc: "9", clang: "9", msvc: "19.25"}
tags: [is_constant_evaluated, 常量求值, constexpr, 求值时机, 编译期分支]
alternatives: [if consteval（C++23）, __builtin_is_constant_evaluated, consteval 函数, 重载分派]
proposal: P0595R2
last_verified: "2026-11-14"
claims:
  - id: c1
    statement: "封装 std::is_constant_evaluated 并为基础内建命令与旧标准库提供回退分支"
    evidence: [ev-fmt-is-constant-evaluated-fallback-001]
  - id: c2
    statement: "用 std::is_constant_evaluated 实现库内兼容封装，旧标准退回内建函数"
    evidence: [ev-abseil-is-constant-evaluated-wrapper-002]
  - id: c3
    statement: "constexpr 辅助函数用 std::is_constant_evaluated 判断当前是否在常量求值中"
    evidence: [ev-glaze-is-constant-evaluated-fastfloat-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

一个 `constexpr` 函数在编译期和运行期都会执行，但两边的能力并不相同。编译期不能做 I/O、不能依赖运行期地址、某些类型转换与内存操作也不可用；运行期则没有这些限制。问题在于函数体无法仅凭签名知道当前是哪一次求值，于是同一份实现要么被迫使用编译期也合法的保守写法，要么对两个时机分别复制。

`std::is_constant_evaluated()` 在常量求值中返回 `true`，在运行期返回 `false`，让一个 `constexpr` 函数可以在内部按求值时机走不同分支。P0595R2 把它加入 C++20，头文件是 `<type_traits>`。它不是“这个函数是不是 `constexpr`”的查询，而是“这一次调用是否发生在常量求值中”的查询。

## 基础用法

`std::is_constant_evaluated()` 是 `constexpr` 函数，直接放在普通 `if` 里即可；条件在每次求值时确定。

<!-- compile -->

```cpp
#include <iostream>
#include <type_traits>

constexpr int path() { return std::is_constant_evaluated() ? 1 : 2; }

int main() {
  constexpr int compile_time = path();
  int runtime = path();
  std::cout << compile_time << ' ' << runtime << '\n';
}
```

输出：

```text
1 2
```

完整版本见 [`examples/cpp20/is_constant_evaluated.cpp`](../../examples/cpp20/is_constant_evaluated.cpp)，六个场景的输出为：

```text
[1] compile-time = 20
[2] runtime = 11
[3] constexpr initializer = true
[4] runtime call = false
[5] compile-time path = 1
[6] runtime path = 2
```

场景 1 的 `constexpr` 初始化让 `evaluate(10)` 在编译期执行，返回 `10 * 2`，并用 `static_assert` 验证。场景 2 传入运行期的值，走 `value + 1` 分支。场景 3 在 `constexpr` 初始化器里直接调用，得到 `true`。场景 4 的普通初始化器得到 `false`。场景 5、6 用同一个 `path_id()` 对照两种求值时机。

## 进阶用法

**为常量求值选择安全实现。** 运行期可以用 `memcpy`、`reinterpret_cast`、日志或分配，编译期不行。把可移植但较慢的分支放在 `is_constant_evaluated()` 为真的路径上，运行期继续走优化版本，是标准库和数值库的常见做法。glaze 的 `cpp20_and_in_constexpr()` 就是一行判断，让 `fast_float` 在常量求值时切到可移植路径。

**兼容旧标准。** 库不能假定所有用户都在 C++20。通常先用 `__cpp_lib_is_constant_evaluated` 判断标准函数是否可用，否则退回 `__builtin_is_constant_evaluated`，再不行就使用调用方给的默认值。fmt 与 abseil 的封装都是这个结构，fmt 还针对 clang 14 与 libstdc++ 的已知不兼容单独绕行。

**只影响当前这一次求值。** 同一个函数模板的两份实例、同一个函数的一次编译期调用和一次运行期调用，结果可以不同。它回答的是“本次求值”，不是类型或对象的固有属性。

**普通 `if`，不是 `if constexpr`。** 条件在编译期调用与运行期调用中取值不同，不能用 `if constexpr` 把某个分支在编译期直接丢弃；运行期那次调用同样需要两个分支都存在。

**C++23 的替代写法。** `if consteval` 用语言语法表达同一件事，`if !consteval` 表示运行期分支；它更直观，也不依赖 `<type_traits>`。新代码若已要求 C++23，优先用 `if consteval`。

## 真实项目证据

| 项目       | 位置                                                                                                                                                                             | 用法                                                            | 为什么典型                                               |
| ---------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------- | -------------------------------------------------------- |
| fmt        | [`include/fmt/base.h#L370-L384`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/base.h#L370-L384)                                       | 封装 `std::is_constant_evaluated`，为内建函数与旧标准库提供回退 | 库要跨多个编译器版本，标准函数、内建函数、默认值三级回退 |
| abseil-cpp | [`absl/meta/type_traits.h#L529-L537`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/meta/type_traits.h#L529-L537)                      | 用标准函数实现兼容封装，旧标准退回编译器内建                    | 特性测试宏选择路径，保持 C++17 与 C++20 下的语义一致     |
| glaze      | [`include/glaze/util/fast_float.hpp#L364-L370`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/util/fast_float.hpp#L364-L370) | `constexpr` 辅助函数判断当前是否在常量求值中                    | 浮点解析在编译期与运行期需要不同实现，用一行判断分流     |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖兼容封装、宏选择与数值解析分流三种用法。

## 陷阱与边界

**它查询的是“这一次求值”。** 把 `std::is_constant_evaluated()` 直接写进普通语句，结果取决于外层表达式是否在常量求值中；不要把它当成常量布尔值缓存起来再到处使用。

**不要用 `if constexpr`。** `if constexpr` 的条件必须是常量表达式，而运行时那次调用的结果不是。写成 `if (std::is_constant_evaluated())`，让两个分支都保留。

**`constexpr` 初始化器会改变结果。** `constexpr bool b = std::is_constant_evaluated();` 为真，而 `bool b = std::is_constant_evaluated();` 为假。区别来自初始化器是否被常量求值，不是变量类型。

**非 `constexpr` 函数里恒为假。** 普通函数的调用永远发生在运行期，判断没有意义。它只对 `constexpr`、`consteval` 以及常量表达式中的调用有价值。

**旧实现的兼容问题。** 早期 clang 与 libstdc++ 的 `consteval` 组合曾出现错误结果，fmt 为此写了专门的绕行。跨编译器时检查特性测试宏，并留意标准库版本。

**与 `consteval` 的分工。** `consteval` 函数强制只在编译期求值，不需要判断；`is_constant_evaluated` 面向的是既能编译期又能运行期执行的 `constexpr` 函数。

## 库联动

- **`constexpr` / `consteval`**：它服务于 `constexpr` 函数的双路径实现；`consteval` 函数不需要判断求值时机。
- **`if consteval`**（C++23）：语言级替代，语法更直接，不再需要 `<type_traits>`。
- **`__builtin_is_constant_evaluated`**：编译器内建，标准函数不可用时作为回退。
- **特性测试宏**：`__cpp_lib_is_constant_evaluated` 用来在编译期判断标准函数是否可用。
- **`std::bit_cast` / `std::memcpy`**：常在运行期分支使用，编译期分支则改成逐字节或逐位操作。

## 横向对比

| 方案                              | 形式       | 判断对象               | 编译期可用 | 主要限制                  |
| --------------------------------- | ---------- | ---------------------- | ---------- | ------------------------- |
| `std::is_constant_evaluated`      | 库函数     | 本次求值是否在常量求值 | 是         | C++20 起；只能用普通 `if` |
| `if consteval`                    | 语言语法   | 同上                   | 是         | C++23 起                  |
| `consteval` 函数                  | 函数说明符 | 强制编译期求值         | 是         | 不能在运行期调用          |
| `__builtin_is_constant_evaluated` | 编译器内建 | 同上                   | 是         | 非标准，依赖具体编译器    |

`std::is_constant_evaluated` 由 P0595R2 进入 C++20，把“当前是不是在编译期求值”从编译器内建提升为可移植的标准函数。它让一个 `constexpr` 函数同时容纳两种实现而无需重载或复制，边界也很清楚：只回答本次求值，不改变类型，也不能替代 `consteval` 的强制语义。
