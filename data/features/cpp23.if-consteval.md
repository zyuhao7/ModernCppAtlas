---
id: cpp23.if-consteval
title: if consteval
standard: C++23
kind: language
category: [编译期计算, 控制流]
difficulty: 3
status: final
tier: A
compilers: {gcc: "12", clang: "14", msvc: "19.44"}
tags: [if consteval, consteval, 常量求值, 编译期求值, constexpr]
alternatives: [std::is_constant_evaluated, if constexpr, 拆分 constexpr 与运行时函数]
proposal: P1938R3
last_verified: "2026-11-18"
claims:
  - id: c1
    statement: "在常量求值时可移植的标量实现与运行期的 SIMD 实现之间用 if consteval 分流"
    evidence: [ev-simdutf-if-consteval-base64-001]
  - id: c2
    statement: "常量求值下不能使用的 memcpy 等运行时操作，用 if consteval 替换为逐字节实现"
    evidence: [ev-glaze-if-consteval-murmur-002]
  - id: c3
    statement: "库用特性宏包装 if consteval，在不支持的编译器上回退到替代写法"
    evidence: [ev-stdexec-if-consteval-macro-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

`constexpr` 函数既可能在编译期求值，也可能在运行期执行。有些操作只能在运行期做，例如 `memcpy`、`malloc`、I/O；另一些实现只适合编译期，例如逐字节循环。C++20 之前只能用 `if constexpr` 加类型或模板技巧，或者调用 `std::is_constant_evaluated()` 在运行时分支里判断，代码意图不直观，而且真分支并不是立即函数上下文。

C++23 的 `if consteval` 直接在函数体内询问“当前是否处于常量求值”。真分支只在常量求值时执行，假分支在运行期执行；条件本身必须是常量表达式，编译器可以在求值阶段就选定一条路径。这样同一个 `constexpr` 函数可以给编译期提供可移植的标量实现，给运行期提供快速路径，而不必拆成两个函数。

## 基础用法

`if consteval` 后面直接跟复合语句，没有括号，也不能写任意条件表达式。下面的函数在常量求值时返回 `value * 2`，在运行期返回 `value + 100`。

<!-- compile -->

```cpp
#include <iostream>

constexpr int compute(int value) {
  if consteval {
    return value * 2;
  } else {
    return value + 100;
  }
}

int main() {
  constexpr int at_compile_time = compute(5);
  std::cout << at_compile_time << '\n';
}
```

输出 `10`。完整版本见 [`examples/cpp23/if_consteval.cpp`](../../examples/cpp23/if_consteval.cpp)，五个场景的输出为：

```text
[1] compile-time branch = 41
[2] runtime branch = 42
[3] constexpr sum = 10
[4] runtime sum = 110
[5] __cpp_if_consteval = 202106
```

场景 1 用常量初始化，走 `if consteval` 真分支，`branch(40)` 得到 41。场景 2 把输入经 `volatile` 变量读入，强制运行期求值，走假分支得到 42。场景 3 和场景 4 对同一个数组求和：编译期走手写展开得到 10，运行期走范围 `for` 后额外加 100 得到 110，两个数字直接证明走了不同分支。场景 5 输出版本宏。

`if consteval` 可以带 `else`，也可以在 C++23 写成 `if !consteval`。真分支是立即函数上下文，因此可以调用 `consteval` 函数；`std::is_constant_evaluated()` 的普通 `if` 分支没有这个性质。

## 进阶用法

**给编译期换可移植实现。** simdutf 的 base64 解码在 `if consteval` 真分支里调用标量实现，运行期继续走 SIMD 分派。常量求值不能执行运行时分派，真分支保证拿到一份能在编译器里算完的代码，两条路径共用同一个入口。

**隔离不能在常量求值中使用的操作。** glaze 的 MurmurHash 读取 4 个字节时，编译期在 `if consteval` 真分支里逐位移位组装整数，运行期在假分支里用 `memcpy`。`memcpy` 不是常量表达式，但把 `if consteval` 包在 `constexpr` 函数里，编译器只会实例化并求值真分支。

**用特性宏做兼容。** stdexec 用 `STDEXEC_IF_CONSTEVAL` 宏包装这个语法：支持时展开为 `if consteval`，旧编译器回退为 `if (std::is_constant_evaluated())` 一类的写法。宏内部还套了 `if constexpr`，避免某个分支在常量求值时被判定为非法。

**配合 `= delete` 或 `unreachable` 收窄路径。** 真分支调用 `consteval` 校验函数、假分支调用运行时断言，是常见的分工；`if consteval` 让“这里只可能在编译期到达”变成可读的控制流，而不是注释约定。

**不要把运行期状态带进真分支。** 真分支里的变量和调用都必须在常量求值中合法；读取函数参数、`this` 的非 const 成员、`new` 出来的对象通常会让编译期求值失败，此时应把逻辑放进假分支。

## 真实项目证据

| 项目               | 位置                                                                                                                                                                                   | 用法                                             | 为什么典型                                                       |
| ------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ | ---------------------------------------------------------------- |
| simdutf/simdutf    | [`include/simdutf/base64_implementation.h#L79-L87`](https://github.com/simdutf/simdutf/blob/59589cd3df6823cbfdbd253cde3377e7c3e1fe9b/include/simdutf/base64_implementation.h#L79-L87)  | 常量求值时走标量 base64 解码，运行期走 SIMD 分派 | 同一函数在编译期必须放弃运行时分派，是 `if consteval` 的核心场景 |
| stephenberry/glaze | [`include/glaze/util/murmur.hpp#L18-L27`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/util/murmur.hpp#L18-L27)                   | 常量求值逐字节组装整数，运行期改用 `memcpy`      | `memcpy` 不能常量求值，展示编译期与运行期实现的切换              |
| NVIDIA/stdexec     | [`include/stdexec/__detail/__utility.hpp#L360-L372`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__utility.hpp#L360-L372) | 用 `STDEXEC_IF_CONSTEVAL` 宏包装，旧编译器回退   | 库需要在特性缺失时降级，说明宏封装这一实际做法                   |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径，覆盖编解码、哈希与库兼容层三种用法。

## 陷阱与边界

**条件必须是常量表达式。** `if consteval` 不接受运行时条件，也不能写成 `if consteval(expr)`；`if !consteval` 只是否定形式，同样没有括号。

**真分支是立即函数上下文。** 这既是能力也是限制：可以调用 `consteval` 函数，但所有语句都必须满足常量求值规则。调试时如果编译期报“不是常量表达式”，先检查真分支里是否混入了运行时操作。

**与 `std::is_constant_evaluated()` 语义不同。** 后者返回 `bool`，分支不会被当作立即函数上下文；在常量求值中调用非法操作时，编译器要到求值阶段才报错，而 `if consteval` 的结构更利于诊断。

**宏回退会改变语义。** 用 `std::is_constant_evaluated()` 模拟 `if consteval` 时，真分支不是立即函数上下文，某些依赖 `consteval` 调用或诊断的代码在旧编译器上行为不同；跨编译器库要单独测试回退路径。

**别在非 `constexpr` 函数里期待编译期分支。** 普通运行时函数调用 `if consteval` 时条件为 false，只会走假分支；它不会因为参数是常量就变成常量求值。

**支持时间。** GCC 12、Clang 14、MSVC 19.44 起提供；特性测试宏为 `__cpp_if_consteval >= 202106L`。

## 库联动

- **`consteval`**：只能常量求值的函数说明符；`if consteval` 真分支可以安全调用它。
- **`constexpr`**：函数同时服务编译期与运行期的前提；`if consteval` 只在这种函数里有意义。
- **`std::is_constant_evaluated()`**：C++20 的运行时探测函数，旧编译器的回退手段，语义与 `if consteval` 不完全等价。
- **`std::unreachable()`**：C++23 的不可达标记，常与 `if consteval` 搭配收窄死分支。
- **`std::format` 的编译期检查**：格式化库利用常量求值在编译期校验格式串，`if consteval` 适合放置这类仅编译期需要的逻辑。

## 横向对比

| 方案                           | 语法直观 | 真分支为立即函数上下文 | 旧编译器可用 | 主要限制                               |
| ------------------------------ | -------- | ---------------------- | ------------ | -------------------------------------- |
| `if consteval`                 | 是       | 是                     | 否           | C++23 起；条件必须是常量表达式         |
| `std::is_constant_evaluated()` | 一般     | 否                     | 是           | 普通 `if` 分支，语义更弱               |
| `if constexpr` + 类型技巧      | 否       | 不适用                 | 是           | 只能按类型或模板参数选择，不看求值阶段 |
| 拆分两个函数                   | 是       | 视实现而定             | 是           | 入口重复，编译期与运行期实现容易漂移   |

`if consteval` 由 P1938R3 进入 C++23，把“当前是否常量求值”提升为语言级控制流。它不改变 `constexpr` 的求值规则，也不替代 `consteval`，但让编译期与运行期共用一份函数体成为可读、可诊断的常规写法。
