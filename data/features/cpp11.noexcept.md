---
id: cpp11.noexcept
title: noexcept 异常说明
standard: C++11
kind: language
category: [语言机制, 错误处理, 性能]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.6", clang: "3.0", msvc: "19.0"}
tags: [noexcept, 异常说明, 移动构造, 异常安全]
alternatives: [throw(), 运行时 try/catch, 不写异常说明]
proposal: N3050
last_verified: "2026-11-04"
claims:
  - id: c1
    statement: "查询接口与 swap 标注 noexcept，声明不抛异常"
    evidence: [ev-drogon-noexcept-result-accessors-001]
  - id: c2
    statement: "string_view 的访问器全部标注 noexcept"
    evidence: [ev-fmt-noexcept-string-view-002]
  - id: c3
    statement: "移动构造与移动赋值声明 noexcept"
    evidence: [ev-abseil-noexcept-move-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++03 的动态异常说明 `throw(A, B)` 在运行期检查，违反时调用 `std::unexpected`，而且编译器难以据此优化，标准库实现也普遍避免使用。调用方想知道一个函数是否可能抛异常，却没有可靠的编译期手段。

`noexcept` 把“不抛异常”变成函数类型的一部分：编译器可以在编译期查询，标准容器可以据此选择移动而非复制，异常安全边界也更清晰。标准提案为 N3050，语法属于核心语言。

## 基础用法

在函数声明或定义末尾写 `noexcept`，表示函数不抛出异常；`noexcept(expr)` 中 `expr` 为假时等价于可能抛出。`noexcept(f(args))` 是运算符，在编译期返回布尔常量。

<!-- compile -->

```cpp
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

class Buffer {
 public:
  Buffer() = default;
  Buffer(const Buffer&) = default;
  Buffer& operator=(const Buffer&) = default;
  Buffer(Buffer&&) noexcept = default;
  Buffer& operator=(Buffer&&) noexcept = default;
};

int may_throw(int value) { return value; }

int main() {
  std::cout << std::is_nothrow_move_constructible<Buffer>::value << ' '
            << noexcept(std::declval<Buffer>()) << '\n';
  std::cout << noexcept(may_throw(1)) << '\n';

  std::vector<Buffer> buffers;
  buffers.reserve(2);
  buffers.emplace_back();
  buffers.emplace_back();
  std::cout << buffers.size() << ' ' << buffers.capacity() << '\n';
}
```

输出 `1 1`、`0` 与 `2 2`。`Buffer` 的移动构造和移动赋值都标注 `noexcept`，因此 `is_nothrow_move_constructible` 为真；`may_throw` 没有异常说明，`noexcept(may_throw(1))` 为假。完整版本见 [`examples/cpp11/noexcept.cpp`](../../examples/cpp11/noexcept.cpp)，五个场景的输出为：

```text
[1] nothrow move: Buffer = true, Thrower = false
[2] noexcept(safe_add) = true, noexcept(may_throw) = false
[3] move_if_noexcept rvalue: Buffer = true, Thrower = false
[4] relay_safe = true, relay_risky = false
[5] noexcept move: moves = 2, copies = 0; throwing move: moves = 0, copies = 2
```

场景 1 对比两个类：`Buffer` 的移动构造标了 `noexcept`，`Thrower` 的没标，`is_nothrow_move_constructible` 给出不同答案。场景 2 用运算符形式查询两个普通函数，`noexcept(expr)` 在编译期求值。场景 3 展示标准库的选择机制：`std::move_if_noexcept` 对 `Buffer` 返回右值引用，对 `Thrower` 退回左值引用。场景 4 让异常说明由表达式推导：`relay_safe` 声明为 `noexcept(noexcept(safe_add(1)))` 因此为真，而 `relay_risky` 依赖一个没有异常说明的声明，结果为假。场景 5 是最直接的性能对照：两次 `reserve` 强制扩容，`noexcept` 移动的类只发生 2 次移动、0 次拷贝，没标的类正好相反，2 次拷贝、0 次移动。

`noexcept` 是承诺而不是检查：函数体内真的抛出异常时，程序会调用 `std::terminate`，不会退回到调用方。标注前要确认函数及其调用的所有代码都不会抛出。

## 进阶用法

移动构造与移动赋值通常应标注 `noexcept`。`std::vector` 扩容时，如果元素的移动构造是 `noexcept`，它会选择移动；否则为避免移动过程中抛异常导致容器处于不一致状态，它可能退回复制。对只管理指针或句柄的类型，这个标注直接影响性能。

析构函数默认隐式 `noexcept`，除非显式写 `noexcept(false)`。析构中抛出异常会在栈展开时触发 `std::terminate`，因此析构函数应吞掉或记录异常，而不是向外抛。

`noexcept` 可以用表达式条件化：`void swap(T& a, T& b) noexcept(noexcept(a.swap(b)));` 表示仅当底层 `swap` 不抛时才承诺不抛。标准库的容器与 `std::pair` 大量使用这种写法。

`noexcept` 运算符与异常说明的语法相同但语义不同：`noexcept(expr)` 在编译期求值，返回 `bool`；函数声明后的 `noexcept` 是接口契约。两者可以组合使用，让泛型代码自动传播异常规格。

C++17 起 `noexcept` 成为函数类型的一部分，函数指针赋值时异常规格必须兼容；这也让模板推导和重载能区分“可能抛”与“不抛”的版本。

## 真实项目证据

| 项目   | 位置                                                                                                                                                         | 用法                                      | 为什么典型                                     |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------- | ---------------------------------------------- |
| drogon | [`orm_lib/src/Result.cc#L121-L134`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/src/Result.cc#L121-L134) | `size`、`swap`、`columns` 标注 `noexcept` | 只转发容器操作，不会失败，标注后接口契约清晰   |
| fmt    | [`include/fmt/base.h#L547-L555`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/base.h#L547-L555)                   | `string_view` 访问器全部 `noexcept`       | 只读视图不可能抛异常，是视图类型的基本约定     |
| abseil | [`absl/status/status.h#L496-L500`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/status/status.h#L496-L500)        | 移动构造与移动赋值标注 `noexcept`         | 只转移内部表示，让标准容器在扩容时优先选择移动 |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖查询接口、视图访问器与移动操作三种典型场景。

## 陷阱与边界

**`noexcept` 是终止承诺。** 函数内抛出异常会直接 `std::terminate`；不要为了性能给可能失败的函数加 `noexcept`，分配、I/O、解析都可能抛异常。

**不是编译期证明。** 编译器不会检查函数体是否真的可能抛异常；`noexcept` 是调用方与实现者之间的契约，标错不会报错，只会在运行期终止。

**虚函数要一致。** 基类虚函数标注 `noexcept` 后，派生类覆盖也必须是 `noexcept`，否则编译失败；反过来基类可能抛、派生类不抛是允许的。

**移动操作标错会掉性能。** 移动构造不标 `noexcept` 时，`std::vector` 等容器在扩容时可能退回复制；对管理资源的类型，这是常见的性能陷阱。

**构造函数抛异常不会终止，除非标注 `noexcept`。** 普通构造函数可以抛；只有显式 `noexcept` 才会把异常变成终止。

**析构函数默认 `noexcept`。** 在析构中抛异常几乎必然终止程序；需要处理失败时用日志或错误码，不要向外抛。

**与 `throw()` 不完全等价。** C++11 的 `throw()` 已弃用，语义接近 `noexcept` 但调用 `std::unexpected`；新代码统一用 `noexcept`。

## 库联动

- **`<type_traits>`**：`is_nothrow_move_constructible`、`is_nothrow_swappable` 等特征让泛型代码查询异常规格。
- **标准容器**：`std::vector` 扩容、`std::move_if_noexcept` 都依赖元素的 `noexcept` 移动构造。
- **`std::swap`**：标准实现按条件 `noexcept`，自定义类型的 `swap` 应与之一致。
- **fmt / abseil**：视图访问器与移动操作在最底层标注 `noexcept`，上层接口可以安全地传播该保证。
- **drogon**：查询接口只读内部指针，标注 `noexcept` 后调用方无需为它们写异常处理。

## 横向对比

| 方案               | 检查时机 | 违反后果          | 可被编译器利用 | 主要代价                         |
| ------------------ | -------- | ----------------- | -------------- | -------------------------------- |
| `noexcept`         | 编译期   | 调用 `terminate`  | 是             | 标错会直接终止程序               |
| 动态异常说明       | 运行期   | 调用 `unexpected` | 弱             | C++11 起弃用，C++17 移除         |
| 不写异常说明       | 无       | 正常传播          | 否             | 调用方无法判断，移动优化可能失效 |
| 运行时 `try/catch` | 运行期   | 局部处理          | 否             | 代码噪音大，无法表达接口契约     |

跨标准演进：C++11 通过 N3050 用 `noexcept` 取代动态异常说明并弃用 `throw()`；C++17 将异常规格纳入函数类型并移除动态异常说明；C++20 没有改变核心语义，只在标准库中继续扩大 `noexcept` 的覆盖范围。
