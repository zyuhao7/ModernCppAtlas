---
id: cpp11.static-assert
title: static_assert
standard: C++11
kind: language
category: [类型系统, 编译期计算]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.3", clang: "2.9", msvc: "16.0"}
tags: [static_assert, 编译期断言, 模板约束, 不变量]
alternatives: [运行期 assert, enable_if, 概念 concepts, 注释约定]
proposal: N1720
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "在模板函数内用带消息的 static_assert 拒绝非法类型"
    evidence: [ev-abseil-static-assert-wrapunique-array-001]
  - id: c2
    statement: "用 static_assert 校验互斥量内部状态枚举的偏序关系"
    evidence: [ev-abseil-static-assert-mutex-ordering-002]
  - id: c3
    statement: "在自实现 make_unique 中用 static_assert 拒绝数组类型"
    evidence: [ev-spdlog-static-assert-make-unique-array-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

运行期 `assert` 只能在程序跑起来之后发现问题，而且要写进可执行文件、可在发布版被关掉。很多错误本质上是编译期就已知的：某个类型不满足要求、某个常量假设不成立、某个平台上的 `sizeof` 与预期不符。这些如果不写断言，只能靠注释约定，出错时表现为莫名其妙的模板报错或运行期崩溃。

`static_assert(条件, 消息)` 在编译期检查一个常量表达式，条件不成立则编译失败并打印消息。它不产生任何运行期代码，也不会被 NDEBUG 关掉。标准提案为 N1720。

## 基础用法

`static_assert` 只能出现在能求值常量表达式的地方：命名空间作用域、类体内、函数体内。条件是编译期布尔量。

<!-- compile -->

```cpp
#include <cstdint>
#include <iostream>
#include <type_traits>

template <typename T>
T checked_square(T value) {
  static_assert(std::is_integral<T>::value, "checked_square requires an integral type");
  return value * value;
}

int main() {
  static_assert(sizeof(std::int32_t) == 4, "int32_t must be 4 bytes");
  std::cout << checked_square(7) << '\n';
}
```

输出 `49`。两条断言都在编译期通过；若把 `checked_square(3.5)` 传入，第二条断言会在编译期报错并显示消息。完整版本见 [`examples/cpp11/static_assert.cpp`](../../examples/cpp11/static_assert.cpp)。

## 进阶用法

`static_assert` 是模板代码里表达约束的早期手段。在模板实例化时检查 `std::is_integral<T>`、`std::is_trivially_copyable<T>` 等条件，可以把难以理解的替换失败变成一行清晰的错误消息。与 `std::enable_if` 相比，`static_assert` 更直接，但它不参与重载决议——条件不满足就是硬错误，而不是“该重载被排除”。

库内部常用 `static_assert` 锁定结构性不变量：枚举取值的顺序关系、某个 `sizeof` 等于预期、缓冲区大小与协议常量一致。这类断言把“代码注释里的约定”变成编译器强制检查，重构时一旦破坏立即报错。

C++17 起允许省略消息：`static_assert(condition);`，消息由编译器根据条件文本生成。C++20 的 concepts 和 `requires` 提供了参与重载决议的约束机制，通常比裸 `static_assert` 更适合约束函数模板；但 `static_assert` 仍适合校验库内部不变量，concepts 无法表达那类断言。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                        | 用法                                   | 为什么典型                             |
| ------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------- | -------------------------------------- |
| abseil | [`absl/memory/memory.h#L71-L76`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/memory/memory.h#L71-L76)                           | 在 `WrapUnique` 中拒绝数组与非对象类型 | 非法类型在实例化阶段即报错，并说明原因 |
| abseil | [`absl/synchronization/mutex.cc#L1747-L1750`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/synchronization/mutex.cc#L1747-L1750) | 校验互斥量内部状态枚举的偏序关系       | 无符号比较依赖取值顺序，断言锁定不变量 |
| spdlog | [`include/spdlog/common.h#L383-L387`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/common.h#L383-L387)                     | 在自实现 `make_unique` 中拒绝数组类型  | 回退实现的限制被编译器强制，避免误用   |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖模板类型约束与内部不变量两类用途。

## 陷阱与边界

**只接受常量表达式。** 条件必须能在编译期求值；用运行期变量做条件会直接编译失败。

**不参与重载决议。** 条件不满足就是硬错误，不会像 `enable_if` 或 concepts 那样“排除该重载、改试其它”。约束函数模板时优先用 concepts。

**消息是给读者看的。** 写清楚“为什么不合法”和“期望什么”，否则面对模板报错时读者仍然无从下手。

**放在头文件要能实例化。** 类模板内的 `static_assert` 只在实例化时求值；如果类型从未实例化，断言不会被检查。

**C++17 前必须写消息。** 早于 C++17 的编译器要求两个参数；跨旧工具链的代码要保留消息。

**不能替代运行期检查。** `static_assert` 只管编译期已知的量，输入数据相关的条件仍需运行期 `assert` 或错误处理。

## 库联动

- **`<type_traits>`**：`std::is_integral`、`std::is_trivially_copyable` 等类型特征提供断言条件。
- **模板库**：容器、智能指针在实例化前用 `static_assert` 拒绝不支持的模板实参。
- **协议与序列化库**：用 `static_assert(sizeof(T) == N)` 锁定与外部格式的二进制布局一致。
- **abseil / spdlog**：内存工具与日志库都用它把内部假设变成编译期检查。

## 横向对比

| 方案            | 检查时机 | 参与重载决议 | 运行期开销 | 主要限制                |
| --------------- | -------- | ------------ | ---------- | ----------------------- |
| `static_assert` | 编译期   | 否           | 无         | 条件不满足即硬错误      |
| 运行期 `assert` | 运行期   | 不适用       | 有         | 可被 NDEBUG 关闭        |
| `enable_if`     | 编译期   | 是           | 无         | 语法冗长，错误信息差    |
| concepts        | 编译期   | 是           | 无         | 只能表达类型/表达式约束 |

跨标准演进：C++11 通过 N1720 引入 `static_assert`；C++17 允许省略消息；C++20 的 concepts 提供参与重载决议的替代约束方式。
