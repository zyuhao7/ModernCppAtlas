---
id: cpp23.unreachable
title: std::unreachable
standard: C++23
kind: library
category: [控制流, 性能]
difficulty: 2
status: final
tier: A
compilers: {gcc: "12", clang: "15", msvc: "19.32"}
tags: [unreachable, 不可达, 控制流, 优化, UB]
alternatives: [__builtin_unreachable, __assume, assert(false), abort, throw]
proposal: P0627R6
last_verified: "2026-11-15"
claims:
  - id: c1
    statement: "枚举全覆盖的 switch 之后调用 std::unreachable，向编译器声明控制流不可达"
    evidence: [ev-hyprland-unreachable-switch-001]
  - id: c2
    statement: "consteval 函数在非法输入分支用特性测试宏选择 std::unreachable，旧标准库回退抛异常"
    evidence: [ev-simdutf-unreachable-consteval-base64-002]
  - id: c3
    statement: "库用特性测试宏与编译器内建组成 std::unreachable 的降级链"
    evidence: [ev-abseil-unreachable-macro-fallback-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

有些控制流分支在逻辑上不可能到达：`switch` 已经覆盖枚举的全部取值，函数却仍要在末尾返回；`if` 链已经穷尽所有情况，编译器依然认为可能落到函数末尾，于是报出“缺少返回值”的警告。开发者通常加一句 `assert(false)`、`abort()` 或随手 `return {}` 来让编译通过，但这些写法有的引入运行时开销，有的掩盖了真正的不可达语义。

`std::unreachable` 是标准库中用来声明“此处不可达”的函数。它定义在 `<utility>` 中，调用即表示程序若执行到该点就是未定义行为；编译器可以据此消除后续代码、省略返回值检查并做更强的优化。P0627R6 把它加入 C++23，让这个原本依赖编译器内建的意图有了可移植的名字。

## 基础用法

在穷尽所有分支的控制流末尾调用 `std::unreachable()`，告诉编译器和读者：正常输入不会走到这里。

<!-- compile -->

```cpp
#include <utility>

enum class State { idle, running };

int code(State s) {
  switch (s) {
    case State::idle: return 0;
    case State::running: return 1;
  }
  std::unreachable();
}

int main() { return code(State::idle); }
```

完整版本见 [`examples/cpp23/unreachable.cpp`](../../examples/cpp23/unreachable.cpp)，六个场景的输出为：

```text
[1] dx east = 1
[2] dx west = -1
[3] name north = north
[4] name south = south
[5] std::unreachable available = true
[6] sum of dx = 0
```

场景 1、2 用 `switch` 计算方向对应的位移，末尾以 `std::unreachable()` 收尾。场景 3、4 在另一个穷尽分支的函数里返回方向名。场景 5 通过 `__cpp_lib_unreachable` 确认标准库提供该函数。场景 6 把所有方向的分量相加，验证分发结果一致。

## 进阶用法

**给穷尽的 `switch` 补上收尾。** Hyprland 的 `CGeometricMovableAnimated::position` 用 `switch` 覆盖几何取值的全部枚举值，每个分支都 `return`，最后仍调用 `std::unreachable()`。这既满足函数的返回要求，也向优化器声明尾部路径不存在。

**在 `consteval` 中表达硬错误。** simdutf 的 `base64_decode_literal` 是编译期函数，解码失败时应当直接终止编译。它用 `__cpp_lib_unreachable >= 202202L` 判断标准是否可用，可用时调用 `std::unreachable()`，旧标准库则退化为 `throw`。这样新工具链得到纯粹的不可达语义，旧工具链仍能编译。

**写成宏里的降级链。** abseil 的 `ABSL_INTERNAL_UNREACHABLE_IMPL()` 先用特性测试宏选择 `std::unreachable()`，再依次回退到 `__builtin_unreachable()`、`__builtin_assume(false)`、`__assume(false)`，最后是空语句。调用方统一使用 `ABSL_UNREACHABLE()`，不必关心编译器差异。

**它不是断言。** `std::unreachable` 不检查条件，也不在运行时报告错误；它只是授权编译器假设该点不可达。需要“这里不应该发生”的运行时诊断时，仍应使用 `assert` 或显式抛出异常。

**可放在 `[[noreturn]]` 辅助函数中。** 若不想在业务代码里直接写调用，可以包装成一个 `[[noreturn]] void fail()`，内部调用 `std::unreachable()`，让调用点更易读。

## 真实项目证据

| 项目              | 位置                                                                                                                                                                                                        | 用法                                                            | 为什么典型                                         |
| ----------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------- | -------------------------------------------------- |
| Hyprland          | [`src/desktop/view/types/GeometricMovableAnimated.cpp#L8-L15`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/desktop/view/types/GeometricMovableAnimated.cpp#L8-L15) | 枚举全覆盖的 `switch` 之后调用 `std::unreachable()` 收尾        | 几何动画取值分发，避免伪警告并让编译器优化尾部路径 |
| simdutf           | [`include/simdutf/implementation.h#L7759-L7771`](https://github.com/simdutf/simdutf/blob/59589cd3df6823cbfdbd253cde3377e7c3e1fe9b/include/simdutf/implementation.h#L7759-L7771)                             | `consteval` 解码失败时用宏选择 `std::unreachable`，否则 `throw` | 编译期字面量解码，非法输入是硬错误，兼容旧标准库   |
| abseil/abseil-cpp | [`absl/base/optimization.h#L212-L221`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/base/optimization.h#L212-L221)                                               | 特性宏 + 编译器内建组成不可达宏的降级链                         | 底层基础设施，需要覆盖 GCC、Clang、MSVC 与旧标准库 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。两处来自生产路径，覆盖枚举分发、编译期解码与底层宏三种用法。

## 陷阱与边界

**执行到它就是未定义行为。** 标准不要求运行时报告错误。如果输入真的越过了所有已检查的分支，程序可能继续执行、崩溃或被优化成难以理解的行为。

**它不是 `assert` 的替代品。** `assert` 在调试构建中检查条件；`std::unreachable` 不做任何检查，读代码的人必须自己确认前提成立。

**不要用它来“修复”缺失的返回。** 如果某个分支真的可能出现，正确做法是补全逻辑或返回错误，而不是用不可达声明把警告压下去。

**对非枚举输入要格外小心。** `switch` 覆盖枚举不代表运行时的值一定在枚举范围内；从外部数据转换来的枚举可能携带非法值，需要先校验。

**优化强度依赖编译器。** 不同编译器对不可达声明的利用程度不同，性能收益不应被当作可移植保证。

**旧工具链没有它。** GCC 12、Clang 15、MSVC 19.32 起提供。更早版本使用 `__builtin_unreachable()` 等内建或宏封装。

## 库联动

- **`__cpp_lib_unreachable`**：特性测试宏，用于判断标准库是否提供 `std::unreachable`。
- **`std::abort` / `std::terminate`**：真正需要终止程序时使用；不可达声明不负责终止。
- **`assert`**：调试构建中的运行时检查，和不可达声明语义不同但常配合使用。
- **`[[noreturn]]`**：标注不返回的函数；把不可达封装成辅助函数时使用。

## 横向对比

| 方案                    | 运行时行为     | 可移植性 | 编译期可用 | 主要限制                           |
| ----------------------- | -------------- | -------- | ---------- | ---------------------------------- |
| `std::unreachable`      | 未定义行为     | 高       | 是         | C++23 起；不检查条件               |
| `__builtin_unreachable` | 未定义行为     | 低       | 是         | GCC/Clang 扩展                     |
| `__assume(false)`       | 未定义行为     | 低       | 是         | MSVC 扩展                          |
| `assert(false)`         | 调试构建中止   | 高       | 否         | `NDEBUG` 下被移除，发布构建无检查  |
| `abort()` / `throw`     | 立即终止或抛出 | 高       | 否         | 引入运行时行为，不能用于纯常量求值 |

`std::unreachable` 由 P0627R6 进入 C++23，为“此路不通”提供了一个标准、可移植且不引入运行时开销的表达。它的代价同样明确：一旦前提被破坏，行为就是未定义。它适合放在已经证明穷尽的分支之后，而不是用来掩盖遗漏的返回路径。
