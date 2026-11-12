---
id: cpp14.init-capture
title: Lambda 初始化捕获
standard: C++14
kind: language
category: [函数式, 语法]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.9", clang: "3.4", msvc: "19.0"}
tags: [lambda, 初始化捕获, 移动捕获, 闭包]
alternatives: [std::bind, 自定义函数对象, 先移动后按值捕获, std::function 包装]
proposal: N3648
last_verified: "2026-11-12"
claims:
  - id: c1
    statement: "std::async 启动文件监视时用初始化捕获移入回调并按地址捕获存活标志"
    evidence: [ev-glaze-init-capture-async-watch-001]
  - id: c2
    statement: "事件循环回调用初始化捕获接管文件描述符与缓冲区列表"
    evidence: [ev-hyprland-init-capture-readable-002]
  - id: c3
    statement: "执行器包装器用初始化捕获绑定自身引用与局部可调用对象"
    evidence: [ev-stdexec-init-capture-executor-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 的 lambda 只能按值或按引用捕获已有变量。这个限制在三种场景里很别扭：需要把 `std::unique_ptr` 一类只能移动的对象送进闭包；需要在捕获时执行一次表达式而不是原样复制；需要给捕获的成员起一个新名字。C++11 时代的变通办法是写一个自定义函数对象，或者用 `std::bind` 拼参数，代码量立刻膨胀。

C++14 的初始化捕获把“捕获”变成“在闭包里初始化一个成员”。写成 `[name = expression]`，`expression` 在定义 lambda 的外层作用域求值，结果成为闭包的成员 `name`。于是移动捕获、表达式捕获和改名捕获都变成同一种语法。

## 基础用法

初始化捕获的左侧是新成员名，右侧是任意可用外层变量构造的表达式。求值发生在外层作用域，lambda 体内看到的是成员。

<!-- compile -->

```cpp
#include <iostream>
#include <string>
#include <utility>

int main() {
  std::string owner = "payload";
  auto take = [text = std::move(owner)] { return text; };
  std::cout << take() << ' ' << owner.empty() << '\n';
}
```

输出：

```text
payload 1
```

完整版本见 [`examples/cpp14/init_capture.cpp`](../../examples/cpp14/init_capture.cpp)，六个场景的输出为：

```text
[1] 12
[2] 100
[3] payload, source null = 1
[4] 12
[5] calls = 3
[6] 2 6 12
```

场景 1 用外层变量计算偏移量后捕获。场景 2 捕获一个表达式的结果，闭包里只保留平方值，不再依赖 `base`。场景 3 把 `std::unique_ptr` 移进闭包，源指针变为空。场景 4 把带移动捕获的 lambda 存进 `std::function`。场景 5 用 `mutable` 让闭包维护自己的计数器。场景 6 在循环里为每个任务捕获当轮的下标与平方值，避免所有闭包共享同一个循环变量。

## 进阶用法

**移动捕获。** `[ptr = std::move(ptr)]` 是初始化捕获最重要的用途。`std::unique_ptr`、`std::promise`、文件句柄等只能移动的类型因此可以进入闭包，而 C++11 的按值捕获要求对象可复制。

**捕获时求值。** 右侧可以是函数调用、算术表达式或其他局部变量。`[sum = a + b]` 只计算一次，之后闭包保存结果；这比在 lambda 体内反复计算更清楚，也能把昂贵的准备步骤移出调用路径。

**改名与解引用。** `[&ref = object]` 可以给引用起别名，`[value = *it]` 可以捕获迭代器指向的值而不是迭代器本身。初始化捕获让捕获列表直接表达“我要保留什么”。

**成员函数中的自我捕获。** `[this]` 捕获指针；`[self = *this]` 复制整个对象，闭包不再依赖原对象存活。C++17 进一步允许 `[*this]` 直接按值捕获当前对象，语义与 `[self = *this]` 相近。

**与 `mutable` 配合。** 初始化捕获的成员默认在 `const` 调用运算符中只读。需要在多次调用之间修改计数、缓存或状态时，把 lambda 声明为 `mutable`。

## 真实项目证据

| 项目           | 位置                                                                                                                                                                                   | 用法                                          | 为什么典型                                   |
| -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------- | -------------------------------------------- |
| glaze          | [`include/glaze/file/file_watch.hpp#L34-L34`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/file/file_watch.hpp#L34-L34)           | `std::async` 任务移入回调并按地址捕获存活标志 | 两种捕获方式并排书写，无需辅助结构           |
| Hyprland       | [`src/render/SurfaceBufferUse.cpp#L38-L39`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/render/SurfaceBufferUse.cpp#L38-L39)                  | 事件循环回调接管文件描述符与缓冲区列表        | 回调晚于函数返回执行，所有权必须转移进闭包   |
| NVIDIA/stdexec | [`include/exec/asio/completion_token.hpp#L476-L478`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/asio/completion_token.hpp#L476-L478) | 按引用绑定自身状态，同时移动局部的可调用对象  | 操作状态不能复制，只能引用；局部对象需要转移 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，分别对应异步任务、事件回调和执行器适配。

## 陷阱与边界

**移动捕获会留下移后源。** `[ptr = std::move(ptr)]` 执行后外层 `ptr` 处于有效但未指定的状态。后续代码不要假设它仍持有原值，必要时重新赋值或让其立即离开作用域。

**闭包复制会复制捕获成员。** 按值或初始化捕获的成员随闭包一起复制；成员只能移动时，整个闭包也只能移动。把这种闭包放进 `std::function` 会编译失败，C++23 的 `std::move_only_function` 才支持只移动的可调用对象。

**初始化表达式在外层作用域求值。** `[x = x]` 右侧的 `x` 是外层变量，不是闭包成员。名字相同容易误读；需要强调时改用不同名字，例如 `[copy = x]`。

**默认捕获不覆盖初始化捕获。** `[=]` 或 `[&]` 只影响隐式捕获，`[name = expression]` 总是显式初始化一个成员。混用时要清楚每个名字的来源。

**修改成员需要 `mutable`。** 不带 `mutable` 时，lambda 的调用运算符是 `const`，捕获成员不可写。计数器、缓存一类状态必须显式声明。

**C++11 不可用。** 初始化捕获是 C++14 特性。面向 C++11 的代码只能用 `std::bind`、自定义函数对象，或先手动移动到一个可复制的包装里。

## 库联动

- **`std::async` / `std::thread`**：启动异步任务时用移动捕获转移资源，用初始化捕获保存运行参数。
- **`std::function`**：可复制的闭包可以存入；只移动的闭包需要 C++23 的 `std::move_only_function`。
- **`std::unique_ptr`**：初始化捕获是让独占所有权进入闭包的标准方式。
- **`std::bind`**：C++11 的替代品；绑定表达式可读性较差，且难以表达移动与改名，通常只在兼容旧标准时使用。

## 横向对比

| 方案                    | 移动捕获 | 捕获时求值 | 可读性 | 主要限制                       |
| ----------------------- | -------- | ---------- | ------ | ------------------------------ |
| `[name = expression]`   | 是       | 是         | 高     | C++14 起                       |
| C++11 按值 / 按引用捕获 | 否       | 否         | 高     | 只能捕获已有变量，不能移动     |
| `std::bind`             | 有限     | 否         | 低     | 占位符与绑定顺序易错，调试困难 |
| 自定义函数对象          | 是       | 是         | 中     | 需要额外类型定义，样板代码多   |

初始化捕获由 N3648 进入 C++14，把“闭包成员如何构造”从语言限制变成普通表达式。它没有引入新的运行时机制，但让移动语义真正进入了 lambda，异步与资源管理代码因此不再需要绕道自定义函数对象。
