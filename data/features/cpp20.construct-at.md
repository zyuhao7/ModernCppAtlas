---
id: cpp20.construct-at
title: std::construct_at
standard: C++20
kind: library
category: [对象模型, 内存管理]
difficulty: 3
status: final
tier: A
compilers: {gcc: "10", clang: "10", msvc: "19.28"}
tags: [construct_at, 就地构造, constexpr, placement new, 生命周期]
alternatives: [placement new, std::allocator::construct, std::uninitialized_default_construct, 直接赋值]
proposal: P0784R7
last_verified: "2026-11-12"
claims:
  - id: c1
    statement: "constexpr 就地容器用 std::construct_at 在尾部构造元素"
    evidence: [ev-mpunits-construct-at-inplace-vector-001]
  - id: c2
    statement: "控制块在原子计数初始化之后用 std::construct_at 延迟构造被管理对象"
    evidence: [ev-stdexec-construct-at-intrusive-ptr-002]
  - id: c3
    statement: "就地容器在原始存储尾部用 std::construct_at 追加元素并更新大小"
    evidence: [ev-glaze-construct-at-inplace-vector-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

原地构造过去只能写 placement new：`new (ptr) T(args...);`。它有两个问题。语法上要 `#include <new>`，位置参数和类型混在一行，`new` 关键字让人误以为在分配内存。语义上更关键：placement new **不能**在常量表达式里使用，于是所有依赖原地构造的容器（`std::vector`、`inplace_vector`、`optional` 一类）都无法在编译期完成部分操作。

C++20 的常量求值能力（P0784R7）解除了部分限制，标准同时给出 `std::construct_at`：一个普通函数，签名是 `T* construct_at(T* ptr, Args&&... args)`，在 `ptr` 指向的存储上构造 `T` 并返回 `ptr`。它与 placement new 语义等价，但**可以在 `constexpr` 函数中调用**，让容器的构造路径进入常量求值。

## 基础用法

`std::construct_at(ptr, args...)` 在 `ptr` 处构造对象，返回该指针。`ptr` 必须指向为 `T` 分配的、对齐正确的原始存储。构造出的对象要用 `std::destroy_at` 销毁。

<!-- compile -->

```cpp
#include <cstddef>
#include <iostream>
#include <memory>

struct Point {
  int x;
  int y;
};

int main() {
  alignas(Point) std::byte storage[sizeof(Point)];
  Point* p = std::construct_at(reinterpret_cast<Point*>(storage), 3, 4);
  std::cout << p->x << ' ' << p->y << '\n';
  std::destroy_at(p);
}
```

输出：

```text
3 4
```

完整版本见 [`examples/cpp20/construct_at.cpp`](../../examples/cpp20/construct_at.cpp)，六个场景的输出为：

```text
[1] point = 3,4
[2] constexpr = 42
[3] strings = hello zzz
[4] slot = 7,8
[5] scalar = 42
[6] moved = 1,2
```

场景 1 在栈上对齐缓冲区里构造 `Point`，用完 `destroy_at`。场景 2 在 `constexpr` lambda 里构造内建数组元素，编译期算出结果。场景 3 在字符串数组上分别用拷贝与 `(count, char)` 构造。场景 4 在 `operator new` 得到的原始内存上构造。场景 5 在已有 `int` 上重新构造并返回指针。场景 6 从另一个 `Point` 构造新对象。

## 进阶用法

**容器实现的核心。** `std::vector`、`std::optional`、自写的 `inplace_vector` 都用它在未初始化缓冲区上构造元素。P0784R7 之后，这些容器的 `constexpr` 版本可以走同一条构造路径，不必为常量求值单独写分支。

**替换 placement new。** `new (ptr) T(args...)` 可以整体换成 `std::construct_at(ptr, args...)`，得到更清晰的调用形式，并顺带获得 `constexpr` 能力（前提是 `T` 的构造函数也是 `constexpr`）。头文件从 `<new>` 换成 `<memory>`。

**返回构造好的指针。** `construct_at` 返回 `T*`，可以直接接续使用或链式调用，省去再写一次强制转换。

**与 `destroy_at` 成对。** 构造和销毁配对出现是生命周期管理的常态。`std::destroy_at(ptr)` 调用析构函数，等价于 `ptr->~T()`，把析构也变成普通函数调用，便于泛型代码统一处理数组与单对象。

**构造顺序可控。** 在构造函数体内而非初始化列表调用 `construct_at`，可以精确控制“先初始化哪些子对象”。stdexec 的侵入式指针即用此特性，确保原子计数先就位、被管理对象后构造。

## 真实项目证据

| 项目           | 位置                                                                                                                                                                                                   | 用法                                                              | 为什么典型                                                            |
| -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------------------- | --------------------------------------------------------------------- |
| mp-units       | [`src/core/include/mp-units/ext/inplace_vector.h#L108-L115`](https://github.com/mpusz/mp-units/blob/27d2def9082ce00d7eb4f75695dbead4a748f23f/src/core/include/mp-units/ext/inplace_vector.h#L108-L115) | `constexpr` 就地容器在尾部构造元素                                | 函数标注 `constexpr`，是 `construct_at` 相对 placement new 的核心优势 |
| NVIDIA/stdexec | [`include/stdexec/__detail/__intrusive_ptr.hpp#L103-L111`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__intrusive_ptr.hpp#L103-L111)     | 控制块在原子计数初始化后用 `std::construct_at` 延迟构造被管理对象 | 构造顺序有语义，必须后于原子成员                                      |
| glaze          | [`include/glaze/containers/inplace_vector.hpp#L261-L262`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/containers/inplace_vector.hpp#L261-L262)   | 就地容器在原始存储尾部追加元素并更新大小                          | 在裸存储上构造，`constexpr` 路径可用                                  |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖常量求值容器、引用计数控制块与序列化容器。

## 陷阱与边界

**地址必须是对齐正确的原始存储。** `ptr` 要指向为 `T` 预留、对齐到 `alignof(T)` 的内存，且该位置没有活跃对象。在 `alignas` 不足的缓冲区上构造是未定义行为。

**它不分配内存。** `construct_at` 只在已有地址上构造，空间由调用方负责。把它们和 `new` 混为一谈会漏掉释放步骤。

**不会自动析构旧对象。** 覆盖已有活跃对象前必须先 `destroy_at`（同一类型可例外，但仍应显式）。跳过析构对持有资源的类型会泄漏。

**`constexpr` 有条件。** `construct_at` 本身可 constexpr，但整条路径要能在常量求值中成立，`T` 的构造函数、所有权转移和后续使用都必须满足常量表达式限制。带 `new`/`delete` 的动态分配要到 C++20 才部分可用，跨标准时需实测。

**异常安全要自己管。** 构造中途抛出时，已经构造的子对象要由调用方清理，`construct_at` 不做回滚。这类容器实现通常用“记录已构造数量”的模式（glaze 的做法）来保证异常安全。

**不要用于有活跃对象的地址。** 在 `T t;` 之后对 `&t` 调用 `construct_at` 会结束旧对象生命周期再构造新对象，对非平凡类型是常见错误来源。

## 库联动

- **`std::destroy_at`**：与 `construct_at` 配对的析构入口，构成完整生命周期管理。
- **`std::uninitialized_default_construct` 等算法**：批量构造未初始化范围，内部即依赖这组原语。
- **`std::allocator` / `std::allocator_traits`**：分配与构造解耦后，构造一端正是 `construct_at` 这类操作。
- **`std::optional` / `std::vector`**：标准容器在 `constexpr` 语境下用它们完成元素生命周期管理。
- **`std::constructible_from`**：概念约束，用于把 `construct_at` 包装进要求可构造的模板。

## 横向对比

| 方案                             | `constexpr` | 返回指针 | 头文件     | 主要限制                           |
| -------------------------------- | ----------- | -------- | ---------- | ---------------------------------- |
| `std::construct_at`              | 是          | 是       | `<memory>` | C++20 起；仍需自行管理存储与析构   |
| placement new                    | 否          | 是       | `<new>`    | 无法用于常量表达式                 |
| `std::allocator::construct`      | 是          | 否       | `<memory>` | C++17 起弃用，仅分配器上下文可用   |
| `std::uninitialized_*_construct` | 是          | 否       | `<memory>` | 面向范围，需要成对逐个构造时更繁琐 |

`std::construct_at` 由 P0784R7 进入 C++20，它是把那批“让容器在编译期可构造”的改动落到 API 表面的一环。相对 placement new，它旅行更少、更易读，最重要的是能在 `constexpr` 里使用；容器作者与需要手工管理生命周期的代码都能直接受益。
