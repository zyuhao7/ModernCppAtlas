---
id: cpp14.exchange
title: std::exchange
standard: C++14
kind: library
category: [对象模型, 性能]
difficulty: 2
status: final
tier: A
compilers: {gcc: "5", clang: "3.4", msvc: "19.0"}
tags: [exchange, 移动语义, 状态重置, 资源转移, 工具函数]
alternatives: [std::swap, 手动保存旧值, std::move 加赋值, getter/setter]
proposal: N3668
last_verified: "2026-11-12"
claims:
  - id: c1
    statement: "用 std::exchange 在移动构造中转移成员指针并重置源对象"
    evidence: [ev-stdexec-exchange-move-reset-001]
  - id: c2
    statement: "用 std::exchange 把析构函数负责释放的裸指针移交给新对象"
    evidence: [ev-nghttp2-exchange-move-reset-002]
  - id: c3
    statement: "用 std::exchange 读取并清零解析器的布尔状态位"
    evidence: [ev-glaze-exchange-flag-reset-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

移动构造和移动赋值经常要做同一件事：把成员从源对象搬到目标对象，再把源对象恢复成可安全析构的状态。手写这段逻辑需要先保存旧值、再写入新值，两步分离；漏掉第二步，源对象仍然持有指针或资源，析构时就可能重复释放。

消费一次性状态也一样。解析器、状态机、缓存常用布尔位表示“上一步发生了什么”，读取之后必须清零，否则状态会泄漏到下一次迭代。先读再写本来只有一行语义，却被迫写成两条语句，还要引入临时变量。

`std::exchange(obj, new_value)` 在一条表达式里完成“取出旧值、写入新值、返回旧值”。它让资源转移和状态重置都变成单步操作，移动构造函数、移动赋值与状态消费都能直接受益。

## 基础用法

`std::exchange(obj, new_value)` 返回 `obj` 的旧值，同时把 `new_value` 赋给 `obj`。返回类型是 `T`，参数按完美转发接收新值。

<!-- compile -->

```cpp
#include <iostream>
#include <string>
#include <utility>

int main() {
  int value = 10;
  int previous = std::exchange(value, 42);
  std::cout << previous << ' ' << value << '\n';

  std::string text = "hello";
  std::string taken = std::exchange(text, std::string{});
  std::cout << taken << ' ' << text.empty() << '\n';
}
```

输出：

```text
10 42
hello 1
```

完整版本见 [`examples/cpp14/exchange.cpp`](../../examples/cpp14/exchange.cpp)，五个场景的输出为：

```text
[1] old = 10, value = 42
[2] taken = hello, source empty = 1
[3] first empty = 1, second size = 4
[4] second empty = 1, third sum = 5
[5] was_dirty = 1, dirty = 0
```

场景 1 是最小形态：交换后 `old` 拿到 10，`value` 变成 42。场景 2 用空字符串交换，原字符串内容被“取走”，源对象变为空。场景 3 的 `Buffer` 移动构造函数用 `exchange` 接管指针和长度，源对象因此处于可析构的空状态。场景 4 的移动赋值先释放目标原有缓冲区，再接管源指针，同样把源重置为空。场景 5 演示状态位消费：`was_dirty` 保存旧值，`dirty` 在同一条语句里清零。

`exchange` 只修改第一个参数。如果只想交换两个对象，用 `std::swap`；如果只需要把对象置空并丢弃旧值，直接赋默认值即可，不必为返回的临时对象付出移动构造的代价。

## 进阶用法

移动构造的成员初始化列表是 `exchange` 最典型的落点：

```cpp
Buffer(Buffer&& other) noexcept
    : data(std::exchange(other.data, nullptr)),
      size(std::exchange(other.size, 0)) {}
```

这里每个成员都独立交换，源对象的指针变成 `nullptr`、长度变成 0，析构函数看到空指针直接跳过。移动赋值则在 `if (this != &other)` 之后先释放目标原有资源，再用同样的方式接管源成员。

状态机与扫描器也常用它消费标志位。读取旧值与清零同时发生，分支可以直接使用两个 `const` 局部变量，不必担心后续代码把状态改回去。若一个循环每轮都要“取走上轮结果并重置”，`exchange` 可以省掉显式的临时变量。

`std::exchange` 在 C++14 中就是 `constexpr`，可以在常量表达式里使用。它对类型的要求很低：`T` 可移动构造，且能由 `U` 赋值。新值可以是同类型对象、`nullptr`、`0`、`std::string{}` 等任何可赋值的值。

## 真实项目证据

| 项目               | 位置                                                                                                                                                                         | 用法                                         | 为什么典型                                            |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------- | ----------------------------------------------------- |
| NVIDIA/stdexec     | [`include/stdexec/stop_token.hpp#L166-L167`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/stop_token.hpp#L166-L167)       | 移动构造用 `exchange` 转移停止状态指针并置空 | 移动源必须保持可析构的默认状态，避免双重引用          |
| nghttp2/nghttp2    | [`src/shrpx_config.cc#L5021-L5023`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/shrpx_config.cc#L5021-L5023)                        | 移动构造把两个 OpenSSL 上下文指针交给新对象  | 析构函数按空指针判断是否释放，`exchange` 直接满足前提 |
| stephenberry/glaze | [`include/glaze/yaml/common.hpp#L1136-L1137`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/yaml/common.hpp#L1136-L1137) | 扫描前读取并清零两个布尔状态位               | 读取旧值与重置状态合并，分支直接复用局部变量          |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，分别对应移动构造、资源所有权转移和解析器状态消费。

## 陷阱与边界

**`exchange` 不是 `swap`。** `std::exchange(a, b)` 返回 `a` 的旧值，只修改 `a`；`std::swap(a, b)` 交换两个对象。把 `exchange` 当 `swap` 用会丢失 `b` 的旧值。

**避免新值引用同一个对象。** `std::exchange(x, x)` 的求值顺序会让新值在旧值被移走之后才使用，结果不符合直觉。需要交换时用 `std::swap`，不要用别名参数调用 `exchange`。

**新值必须可赋值。** `exchange` 要求目标类型能由新值赋值，同时能移动构造出返回值。传入不可赋值类型会在模板实例化时失败，错误信息指向 `exchange` 内部而不是调用点。

**重置成有效状态。** 移动构造里通常把指针置 `nullptr`、容器置空、整数置 0；随意写入无关值可能让析构函数或后续成员函数进入未定义行为。

**注意返回值的代价。** `exchange` 按值返回旧对象，昂贵类型会产生一次移动构造。只关心新值时直接赋值，不要为了“顺手”多拿一份旧值。

**常量与 `const` 对象不可交换。** 第一个参数是非 `const` 左值引用，`const` 对象无法传入。需要读取旧值又不想修改对象时，先复制再判断。

## 库联动

- **`std::unique_ptr` / `std::shared_ptr`**：`exchange(ptr, nullptr)` 可以在不引入中间变量的情况下交出所有权。
- **`std::optional`**：`exchange(opt, std::nullopt)` 取出旧的可选值并把对象清空。
- **标准容器**：`exchange(v, std::vector<T>{})` 返回旧缓冲区并让容器恢复空状态，适合复用与清理。
- **解析器与状态机**：标志位在消费后立即清零，避免跨输入泄漏（glaze 的 YAML 扫描器即此用法）。

## 横向对比

| 方案                    | 返回旧值 | 重置源对象 | 语句数 | 主要限制                 |
| ----------------------- | -------- | ---------- | ------ | ------------------------ |
| `std::exchange(obj, v)` | 是       | 是         | 1      | 返回旧值需要移动构造     |
| `std::swap(a, b)`       | 否       | 否         | 1      | 交换两个对象，不返回旧值 |
| 保存临时变量再赋值      | 是       | 手动       | 2      | 容易漏掉重置，顺序易写错 |
| `std::move(obj)` 后清空 | 否       | 手动       | 2      | 不返回旧值，需额外保存   |

`std::exchange` 由 N3668 进入 C++14，和 `std::make_unique` 一样属于“把常见手写模式标准化”的小工具。C++11 项目可以用两行临时变量实现同样的语义，但代码审计时更容易漏掉源对象重置。
