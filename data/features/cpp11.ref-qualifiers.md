---
id: cpp11.ref-qualifiers
title: 引用限定成员函数（ref-qualifiers）
standard: C++11
kind: language
category: [对象模型, 语言机制]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.8.1", clang: "2.9", msvc: "19.0"}
tags: [引用限定符, 值类别, 移动语义, 重载]
alternatives: [独立取值函数, const 重载, 返回值而非引用, 手工调用 std::move]
proposal: N2439
last_verified: "2026-11-07"
claims:
  - id: c1
    statement: "按值类别提供 const&、&、const&&、&& 四个 get 重载"
    evidence: [ev-abseil-ref-qualified-get-001]
  - id: c2
    statement: "链式构建器按左值/右值分别返回引用"
    evidence: [ev-abseil-ref-qualified-builder-002]
  - id: c3
    statement: "协程结果访问器按对象值类别返回引用或值"
    evidence: [ev-drogon-ref-qualified-awaiter-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

成员函数可以对 `const` 对象与非 `const` 对象重载，但无法区分“对象本身是左值还是右值”。于是同一个访问器在临时对象上调用时，只能返回左值引用，无法把内部资源移动出去，只能复制。C++11 的移动语义让这种区别变得重要：从临时对象取成员应当允许移动，从左值对象取成员必须保留原对象。

C++11 引入引用限定符：在成员函数的参数列表后写 `&` 或 `&&`，让重载决议同时考虑调用对象的值类别。标准提案为 N2439；它与移动构造函数、移动赋值一起，构成完整的值类别感知接口。

## 基础用法

在 `const` 限定符之后追加 `&` 或 `&&`，两个重载按对象是左值还是右值选择。

<!-- compile -->

```cpp
#include <iostream>
#include <string>
#include <utility>

class Message {
 public:
  std::string& text() & { return text_; }
  std::string text() && { return std::move(text_); }

 private:
  std::string text_ = "atlas";
};

int main() {
  Message message;
  std::cout << message.text() << '\n';
  std::string moved = Message{}.text();
  std::cout << moved << '\n';
}
```

输出两行 `atlas`。`message.text()` 命中 `&` 重载并返回引用；`Message{}.text()` 命中 `&&` 重载，直接把成员移动出来。完整版本见 [`examples/cpp11/ref_qualifiers.cpp`](../../examples/cpp11/ref_qualifiers.cpp)，四个场景的输出为：

```text
[1] after mutation = atlas!
[2] moved = atlas
[3] lvalue, const lvalue, rvalue, const rvalue
[4] lvalue yields string& = 1, rvalue yields string = 1
```

场景 1 证明左值重载返回的是成员引用：通过引用追加 `!` 后，原对象的内容也随之改变。场景 2 在临时对象上调用，`&&` 重载把成员移动出来，不需要复制。场景 3 的 `Probe` 写全了 `&`、`const&`、`&&`、`const&&` 四个重载，四种调用分别打出自己的标签，说明重载决议同时考虑值类别与 `const`。场景 4 用 `decltype` 验证返回类型确实随值类别变化：左值得到 `std::string&`，右值得到按值返回的 `std::string`。

## 进阶用法

**四个重载可以齐全。** `const&`、`&`、`const&&`、`&&` 分别对应四种值类别与 cv 组合；abseil 的 `Storage::get` 就是完整四件套，右值路径返回可移动引用。

**`const&&` 也有意义。** 它对应“不能修改但可以取走的临时对象”，常见于完美转发链路的末端；是否单独实现取决于接口是否需要区分。

**链式构建器区分左值与右值。** 左值调用返回 `T&`，临时对象上的链式调用返回 `T&&`，后者可以继续移动，避免每步都复制。

**引用限定与 `const` 是正交的。** `const&` 表示“从 const 左值调用”，`&&` 表示“从非 const 右值调用”；两者可组合，不能只写值类别而忽略 cv。

**对非成员函数不适用。** 引用限定只修饰成员函数；自由函数需要区分值类别时，用 `&&` 参数或转发引用表达。

**运算符同样可以限定。** `operator()`、赋值运算符等成员运算符都能加引用限定，让 lambda 风格的调用对象只接受特定值类别。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                          | 用法                                   | 为什么典型                              |
| ------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------- | --------------------------------------- |
| abseil | [`absl/container/internal/compressed_tuple.h#L90-L93`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/container/internal/compressed_tuple.h#L90-L93) | `get` 提供四组 cv 与值类别重载         | 完整覆盖 `const&`、`&`、`const&&`、`&&` |
| abseil | [`absl/status/status_builder.h#L200-L202`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/status/status_builder.h#L200-L202)                         | 链式方法按左值/右值返回引用            | 临时对象链式调用可继续移动              |
| drogon | [`lib/inc/drogon/utils/coroutine.h#L234-L248`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/utils/coroutine.h#L234-L248)            | 协程结果访问器按值类别返回引用或移动值 | 直接决定结果所有权是否转移              |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。前两条来自 abseil 的容器与状态设施，第三条来自 drogon 的协程封装，均为生产路径。

## 陷阱与边界

**引用限定不改变函数签名中的参数。** 它只影响重载决议；同一组参数配合不同引用限定可以共存，但省略引用限定的重载会与非引用限定版本冲突。

**忘记写 `const` 可能调用到错误重载。** `const` 对象只能调用 `const&` 或 `const&&` 重载；如果只实现了非 const 版本，编译失败。

**`&&` 重载里要显式 `std::move`。** 右值限定只说明对象可以被移动，不会自动把成员变成右值；返回成员时仍需 `std::move`。

**过度重载会膨胀接口。** 四个版本并不总是必要；多数类型只需要 `const&` 与 `&&` 两个版本即可覆盖常见调用。

**与 `std::move` 后使用同源问题。** 从右值对象移走成员后，原对象处于有效但未指定状态；不要把被移动过的临时对象再用于其他逻辑。

**C++11 编译器支持不完整。** GCC 4.8.1、Clang 2.9 起支持；MSVC 直到 VS2015（19.0）才完整实现，跨平台库常用宏降级。

## 库联动

- **`<utility>`**：右值限定的访问器通常与 `std::move` 成对出现。
- **移动构造与移动赋值**：引用限定让取值接口与移动语义配合，避免把临时对象的资源复制出去。
- **`std::exchange`**：需要“取走并重置”时，可在 `&&` 重载里结合 `std::exchange` 实现。
- **完美转发**：转发引用把值类别带到末端，最终由引用限定重载接住。
- **协程与 future 风格接口**：结果访问器按值类别返回引用或值，drogon 的 `result()` 是直接例子。
- **abseil / drogon**：提供容器访问器、构建器与协程结果三类生产用法。

## 横向对比

| 方案                 | 值类别感知 | 能否移动成员     | 主要限制                        |
| -------------------- | ---------- | ---------------- | ------------------------------- |
| 引用限定重载         | 完整       | 可以，`&&` 路径  | 接口数量增加，需要 C++11 编译器 |
| 只提供 `const&` 访问 | 无         | 不可以，只能复制 | 临时对象上产生不必要的复制      |
| 按值返回             | 部分       | 依赖移动构造     | 左值调用也会复制或移动          |
| 手工调用 `std::move` | 调用方决定 | 可以             | 容易误移左值，语义分散在调用点  |

跨标准演进：C++11 通过 N2439 引入引用限定符，与移动语义同期落地；C++14 的泛型 lambda 与 C++20 的约束一起，让值类别感知的泛型接口更易编写；引用限定语法本身此后未再调整。
