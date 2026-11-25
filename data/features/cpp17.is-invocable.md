---
id: cpp17.is-invocable
title: std::is_invocable 与 std::invoke_result
standard: C++17
kind: library
category: [类型工具, 泛型编程]
difficulty: 3
status: final
tier: A
compilers: {gcc: "7.1", clang: "6", msvc: "19.11"}
tags: [is_invocable, invoke_result, 类型特征, SFINAE, 可调用性]
alternatives: [手写 SFINAE, decltype 探测, std::void_t, requires 表达式]
proposal: P0604R0
last_verified: "2026-11-24"
claims:
  - id: c1
    statement: "按回调是否可用给定参数调用在编译期选择分支"
    evidence: [ev-glaze-is-invocable-cli-001]
  - id: c2
    statement: "把 is_invocable_v 封装成可复用的 C++17 约束谓词"
    evidence: [ev-abseil-is-invocable-requires-002]
  - id: c3
    statement: "用 regular_invocable 与 invoke_result_t 约束运算的结果类型"
    evidence: [ev-mpunits-invoke-result-quantity-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

泛型代码经常要问两个问题：某个可调用对象能不能用这组参数调用？调用后返回什么类型？C++11 时期没有直接的工具，只能借助 `decltype` 配合 SFINAE 手写探测器，或者用 `std::result_of`（要求以函数类型书写，规则晦涩，后来在 C++17 被弃用）。

`std::is_invocable` 与 `std::invoke_result`（P0604R0）把这两件事标准化：`is_invocable<F, Args...>` 给出能否调用的布尔结果，`invoke_result<F, Args...>` 给出调用表达式的类型。两者都按 `std::invoke` 的规则判定，覆盖普通函数、函数对象、成员函数指针和成员数据指针，并配有 `_v` 与 `_t` 变量/别名模板。

## 基础用法

模板参数顺序是“可调用对象在前，实参类型在后”。结果在编译期求值，因此可以直接放进 `if constexpr` 或 `static_assert`。

<!-- compile -->

```cpp
#include <type_traits>

int twice(int value) { return value * 2; }

int main() {
  static_assert(std::is_invocable_v<decltype(&twice), int>);
  static_assert(!std::is_invocable_v<decltype(&twice), int, int>);
  static_assert(std::is_same_v<std::invoke_result_t<decltype(&twice), int>,
                               int>);
}
```

完整示例见 [`examples/cpp17/is_invocable.cpp`](../../examples/cpp17/is_invocable.cpp)，覆盖六种场景，输出为：

```text
[1] twice(int) = 1, twice(string) = 0
[2] join(as, as) = 1, join(int, int) = 0
[3] add(int) member = 1
[4] lambda(int) = 1, lambda() = 0
[5] invoke_result_t<twice, int> is int = 1
[6] is_invocable_r<int, twice, int> = 1, is_invocable_r<double, twice, int> = 1
```

场景 1、2 用自由函数分别检查参数类型是否匹配。场景 3 对成员函数指针检查，`is_invocable` 按 `std::invoke` 规则把对象作为隐式第一个参数。场景 4 检查捕获 lambda 的调用。场景 5 用 `invoke_result_t` 取返回类型并比对。场景 6 用 `is_invocable_r` 检查返回类型能否转换为目标类型。

## 进阶用法

**作为 `if constexpr` 的分流条件。** 编译期判断某种调用合法时，把断言直接放进模板分支。`using R = std::invoke_result_t<F, Args...>` 只有在可调用时才成立，所以先用 `is_invocable_v` 判定，再取返回类型，二者配对使用最安全。

**封装成可复用谓词。** C++17 没有 concepts 时，常见做法是把 `is_invocable_v<F, T...>` 包成一个 `constexpr` 函数或变量模板，再交给 `std::enable_if_t`。Abseil 的 `Requires<T...>(F)` 就是这种模式，让约束写在函数签名里而不暴露 SFINAE 细节。

**成员指针与隐式对象参数。** `is_invocable<decltype(&C::m), C&, ...>` 把对象引用作为第一个模板实参。省略对象参数会得到 `false`，这是判断“是不是成员函数”的常见手法之一。

**注意 `_r` 变体的转换规则。** `is_invocable_r<R, F, Args...>` 在“可调用且返回值能转换为 `R`”时为真。因此 `is_invocable_r<double, twice, int>` 对返回 `int` 的函数也为真，因为 `int` 可转换为 `double`。要精确匹配返回类型应改用 `is_same_v<invoke_result_t<...>, R>`。

## 真实项目证据

| 项目               | 固定源码                                                                                                                                                                                               | 用法                                                 | 为什么典型                                       |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ---------------------------------------------------- | ------------------------------------------------ |
| stephenberry/glaze | [`include/glaze/ext/cli_menu.hpp#L161-L162`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/ext/cli_menu.hpp#L161-L162)                             | 用 `is_invocable_v` 选无参回调分支并取返回类型       | 命令行注册器按签名在编译期分流，避免写多条重载   |
| abseil/abseil-cpp  | [`absl/meta/internal/requires.h#L58-L61`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/meta/internal/requires.h#L58-L61)                                    | 把 `is_invocable_v` 封成 `Requires` 谓词             | C++17 无 concepts，用谓词喂给 `enable_if` 做约束 |
| mpusz/mp-units     | [`src/core/include/mp-units/framework/quantity.h#L113-L115`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/core/include/mp-units/framework/quantity.h#L113-L115) | 用 `regular_invocable` 与 `invoke_result_t` 约束运算 | 概念先判可调用性，再检查返回类型满足表示约束     |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- `is_invocable` 只回答“能否编译通过地调用”，不检查调用后行为是否正确。一个接受任意参数的模板函数对几乎任何实参都返回 `true`，此时它几乎不提供约束力。
- 按 `std::invoke` 规则判定，而不是按普通函数调用。成员函数指针、成员数据指针都要求显式或隐式地把对象作为第一个实参；遗漏对象参数会得到 `false`。
- 返回值只做“可转换性”判断时用 `_r` 变体；需要精确类型时用 `invoke_result_t` 配合 `is_same`/`is_convertible`，不要混用两种语义。
- 对不完整类型、抽象类或含未定义成员指针的表达式调用，结果是 `false` 且不报错，容易掩盖真实的类型错误。
- `invoke_result_t` 在不可调用时是替换失败而非编译错误，只能放在 `if constexpr` 的受保护分支或 SFINAE 上下文中直接取用。
- C++20 起优先用 `std::invocable` 概念与 `requires` 表达式，约束写在概念里比堆叠类型特征更易读。

## 库联动

- [std::invoke 与 std::apply](cpp17.invoke-apply.md) 是同一套调用规则的可执行版本；`is_invocable` / `invoke_result` 判断的正是 `std::invoke` 能否成功。
- [std::void_t](cpp17.void-t.md) 常与这些特征组合，用于检测成员是否存在。
- **`std::function`**：构造时依赖可调用性；`is_invocable` 可在包装前做前置检查。
- **concepts**：C++20 的 `std::invocable`、`std::regular_invocable` 与 `invoke_result_t` 是本页设施的概念化封装。

## 横向对比

| 方案                   | 表达力   | 可读性 | 引入版本 | 主要限制                     |
| ---------------------- | -------- | ------ | -------- | ---------------------------- |
| `std::is_invocable`    | 能否调用 | 高     | C++17    | 不检查返回类型转换以外的约束 |
| `std::invoke_result_t` | 返回类型 | 高     | C++17    | 不可调用时是替换失败         |
| `std::result_of`       | 返回类型 | 低     | C++11    | C++17 弃用、C++20 移除       |
| 手写 SFINAE 探测       | 任意     | 低     | C++11    | 冗长、易错、难维护           |
| `requires` 表达式      | 任意约束 | 高     | C++20    | 需要 C++20 工具链            |

`std::is_invocable` 与 `std::invoke_result` 由 P0604R0 进入 C++17，取代了规则晦涩的 `std::result_of`。它们适合写重载分发、约束模板参数与包装可调用对象；在 C++20 语境里通常让位给 concept，但作为底层构件仍然常见于库内部。
