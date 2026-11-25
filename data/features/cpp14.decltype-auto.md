---
id: cpp14.decltype-auto
title: decltype(auto)（保留引用的返回类型推导）
standard: C++14
kind: language
category: [类型推导, 语法]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.9", clang: "3.4", msvc: "19.0"}
tags: [decltype(auto), 返回类型推导, 完美转发, 引用折叠]
alternatives: [auto 返回值, 尾置 decltype, 显式引用返回, std::invoke 包装]
proposal: N3638
last_verified: "2026-11-25"
claims:
  - id: c1
    statement: "用 decltype(auto) 转发 std::apply 的返回值，保留引用返回值"
    evidence: [ev-abseil-decltype-auto-apply-001]
  - id: c2
    statement: "递归选择可调用对象时用 -> decltype(auto) 保留被选中回调的返回类型"
    evidence: [ev-stdexec-decltype-auto-invoke-002]
  - id: c3
    statement: "格式化辅助函数用 decltype(auto) 返回 func 的精确结果类型"
    evidence: [ev-mpunits-decltype-auto-prefix-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 的 `auto` 返回类型推导按值工作：函数写 `auto f()` 时，即使返回表达式是左值引用，推导出的返回类型也会丢掉引用，调用方拿到的是副本。需要保留引用时，只能写尾置返回类型 `-> decltype(expr)`，但表达式往往很长，或者要重复函数体里的写法。

C++14 的 `decltype(auto)`（N3638）把 `auto` 的简洁和 `decltype` 的精确性合在一起：返回类型按 `decltype(返回表达式)` 推导，引用和值类别都原样保留。它主要出现在转发包装、泛型访问器和需要返回容器元素引用的地方。

## 基础用法

`decltype(auto)` 可以修饰函数返回类型、变量和 lambda 的返回类型。理解它的关键是 `decltype` 的两条规则：未加括号的 id-expression 得到变量声明类型，加括号的表达式按值类别得到引用类型。

<!-- compile -->

```cpp
#include <type_traits>

int counter = 0;
int& slot() { return counter; }

auto copy_of_slot() { return slot(); }
decltype(auto) ref_to_slot() { return slot(); }
decltype(auto) parenthesized() { return (counter); }
decltype(auto) plain() { return counter; }

static_assert(std::is_same<decltype(copy_of_slot()), int>::value, "");
static_assert(std::is_same<decltype(ref_to_slot()), int&>::value, "");
static_assert(std::is_same<decltype(parenthesized()), int&>::value, "");
static_assert(std::is_same<decltype(plain()), int>::value, "");
```

完整示例见 [`examples/cpp14/decltype_auto.cpp`](../../examples/cpp14/decltype_auto.cpp)，输出为：

```text
[1] auto return is int = 1
[2] decltype(auto) return is int& = 1
[3] counter after writing through alias = 7
[4] return (counter) is int& = 1
[5] return counter is int = 1
[6] forwarding wrapper keeps int& = 1
```

场景 1 与 2 对比 `auto` 和 `decltype(auto)` 的返回类型：前者是 `int`，后者是 `int&`。场景 3 通过返回的引用写入 `counter`，值变成 7。场景 4 与 5 展示括号的影响：`(counter)` 是左值表达式，推导为 `int&`；`counter` 是 id-expression，推导为 `int`。场景 6 用转发包装调用返回引用的函数，包装层没有丢掉引用。

## 进阶用法

**把返回表达式原样交给 `decltype`。** `decltype(auto)` 不是“更聪明的 auto”，而是把 `decltype` 的规则套在返回表达式上。函数体 `return expr;` 中 `expr` 的括号、值类别和类型共同决定结果；换成 `return (expr);` 常常改变返回类型。

**写转发包装时优先用它。** 包装 `std::apply`、`std::invoke` 或用户回调时，包装层不应改变返回类型。`decltype(auto)` 让引用、`const` 和 `volatile` 都透传，调用方看到的行为与被包装函数一致；abseil 的兼容层就靠它保持 `std::apply` 的语义。

**返回值推导要求所有 return 一致。** 一个函数里多条 `return` 必须推导出同一类型；`return value;` 与 `return (value);` 混用会得到 `int` 和 `int&`，编译失败。递归函数仍要显式写出返回类型，因为推导发生在函数体解析之后。

**不能从花括号初始化列表推导。** `decltype(auto) f() { return {1, 2}; }` 无法推导，`decltype` 对 braced-init-list 没有类型；需要返回聚合时写显式返回类型或 `std::array`。

**与 `auto&&` 分清职责。** `auto&&` 是变量声明层面的转发引用；`decltype(auto)` 决定函数返回类型。前者不会延长临时对象寿命到调用方，后者返回引用时同样不负责生命周期。

## 真实项目证据

| 项目              | 固定源码                                                                                                                                                                                                       | 用法                                                     | 为什么典型                                     |
| ----------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------- | ---------------------------------------------- |
| abseil/abseil-cpp | [`absl/utility/utility.h#L40-L46`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/utility/utility.h#L40-L46)                                                          | 用 `decltype(auto)` 转发 `std::apply` 的返回值           | 兼容包装必须原样保留被包装函数的结果类型       |
| NVIDIA/stdexec    | [`include/stdexec/functional.hpp#L303-L314`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/functional.hpp#L303-L314)                                         | 递归选择可调用对象时用 `-> decltype(auto)`               | 被选中回调可能返回引用，包装层不能改变值类别   |
| mpusz/mp-units    | [`src/systems/include/mp-units/systems/si/prefixes.h#L124-L132`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/systems/include/mp-units/systems/si/prefixes.h#L124-L132) | 格式化辅助函数用 `decltype(auto)` 返回 `func` 的结果类型 | 结果可能是引用或轻量视图，按值返回会多一次拷贝 |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **悬垂引用。** 返回 `decltype(auto)` 且推导为引用时，引用指向的临时对象可能在函数返回后销毁；`decltype(auto) f() { return local; }` 返回的引用立即悬垂。
- **多条 return 必须同型。** 推导只看每个 `return` 表达式；返回 `int` 与 `int&` 混用会编译失败，不能靠隐式转换统一。
- **括号会改变结果。** `return x;` 与 `return (x);` 分别得到值类型和引用类型；这是刻意的语言规则，不是编译器差异。
- **不能用于花括号列表。** `return {1, 2};` 无法推导，需显式返回类型。
- **变量用法同样精确。** `decltype(auto) x = f();` 在 `f` 返回引用时让 `x` 成为引用；想要副本时写 `auto`。
- **递归与先使用后定义。** 返回类型推导需要看到函数体；递归函数、在类内互相调用的函数仍需显式返回类型。
- **工具链下限。** GCC 4.9、Clang 3.4、MSVC 19.0 起支持；更早的工具链只能用尾置 `decltype`。

## 库联动

- [auto 与 decltype](cpp11.auto-decltype.md)：`decltype` 的推导规则是理解 `decltype(auto)` 的前提。
- [尾置返回类型](cpp11.trailing-return-type.md)：C++11 中保留引用返回的旧写法，常与本页设施互相替代。
- [转发引用](cpp11.forwarding-references.md)：转发包装的参数侧机制，与返回侧的 `decltype(auto)` 配套。
- [std::invoke / std::apply](cpp17.invoke-apply.md)：最常见的被包装对象，返回值通过 `decltype(auto)` 透传。
- [模板 lambda](cpp20.template-lambda.md)：泛型回调常与 `decltype(auto)` 返回类型一起出现。
- **`std::invoke_result`**：编译期查询调用结果类型，是手写推导之外的显式替代。

## 横向对比

| 方案                     | 引入版本 | 保留引用 | 可读性 | 主要限制                       |
| ------------------------ | -------- | -------- | ------ | ------------------------------ |
| `auto` 返回类型          | C++11    | 否       | 高     | 引用和 cv 限定被剥离           |
| 显式返回类型             | C++98    | 是       | 中     | 表达式复杂时要重复写           |
| 尾置 `-> decltype(expr)` | C++11    | 是       | 低     | 表达式长，容易与函数体不一致   |
| `decltype(auto)`         | C++14    | 是       | 高     | 括号与多条 return 的规则要记牢 |
| `decltype(expr)` 变量    | C++11    | 是       | 中     | 只推导变量，不推导函数返回值   |

`decltype(auto)` 把“按值推导”和“按 `decltype` 推导”分开：需要副本时写 `auto`，需要原样透传时写 `decltype(auto)`。它适合转发包装、泛型访问器与返回容器元素引用的 API；普通业务函数明确返回类型通常更易读。
