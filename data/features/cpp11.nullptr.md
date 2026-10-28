---
id: cpp11.nullptr
title: nullptr
standard: C++11
kind: language
category: [类型系统, 语法]
difficulty: 1
status: final
tier: A
compilers: {gcc: "4.6", clang: "2.9", msvc: "16.0"}
tags: [nullptr, 空指针, "NULL", 重载决议, 类型安全]
alternatives: [NULL 宏, 字面量 0, std::nullptr_t 重载, 可选值 std::optional]
proposal: N2431
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用 nullptr 作为事件循环指针参数的默认值"
    evidence: [ev-drogon-nullptr-loop-default-001]
  - id: c2
    statement: "查找失败时返回 nullptr 表示没有对应 logger"
    evidence: [ev-spdlog-nullptr-missing-logger-002]
  - id: c3
    statement: "用 nullptr 初始化可选的输出节指针成员"
    evidence: [ev-mold-nullptr-section-member-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C 和早期 C++ 用 `NULL` 表示空指针，但它通常只是宏展开成的整数 `0`。`0` 可以匹配整型、浮点、指针等多种重载，重载决议经常选错；模板实参推导会把 `0` 推成 `int`，把 `NULL` 推成 `long` 或其它整数类型。更隐蔽的是，函数重载同时接受 `int` 和指针时，传 `NULL` 不报错却调用了错误版本。

`nullptr` 是类型为 `std::nullptr_t` 的字面量。它能转换为任意指针类型和成员指针类型，但不能转换为整数；在重载决议中明确表示“空指针”，模板推导也会得到 `std::nullptr_t` 而不是某个整数类型。标准提案为 N2431。

## 基础用法

`std::nullptr_t` 定义在 `<cstddef>`。`nullptr` 可直接初始化指针、作为函数实参或默认值。

<!-- compile -->

```cpp
#include <cstddef>
#include <iostream>
#include <type_traits>

template <typename T>
bool empty(T* ptr) {
  return ptr == nullptr;
}

void consume(int*) { std::cout << "pointer\n"; }
void consume(std::nullptr_t) { std::cout << "null\n"; }

int main() {
  int* ptr = nullptr;
  static_assert(std::is_same<decltype(nullptr), std::nullptr_t>::value,
                "nullptr has its own type");

  std::cout << empty(ptr) << ' ';
  consume(nullptr);
}
```

输出 `1 null`。`nullptr` 有独立类型，`consume` 的重载按 `std::nullptr_t` 精确匹配而不是猜测整数。完整版本见 [`examples/cpp11/nullptr.cpp`](../../examples/cpp11/nullptr.cpp)。

## 进阶用法

`nullptr` 可以转换为任何对象指针、函数指针和成员指针，转换结果保证是空指针值。它也能用于布尔上下文，`if (ptr)` 与 `if (ptr != nullptr)` 等价。由于不能转换为整数，`int n = nullptr` 会直接编译失败，这正是与 `0` 的关键区别。

模板代码里，`std::nullptr_t` 可以作为独立的重载参数，把“传入空指针”与“传入整数零”区分开。智能指针的默认构造、`reset()` 和比较都接受 `nullptr`；`std::function`、`std::shared_ptr` 等类型也可以直接与 `nullptr` 比较。

`nullptr` 不是指针，不能解引用、做算术或取成员；它只是一个可转换为指针的空值字面量。需要“可能没有值”的语义时，现代 C++ 更推荐 `std::optional<T>` 或 `std::unique_ptr<T>`，而不是裸露的可空指针。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                            | 用法                                  | 为什么典型                               |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------- | ---------------------------------------- |
| drogon | [`lib/inc/drogon/HttpClient.h#L266-L270`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/HttpClient.h#L266-L270)        | 用 `nullptr` 作为事件循环指针默认值   | 默认参数明确表达“未指定”，类型安全       |
| spdlog | [`include/spdlog/details/registry-inl.h#L86-L90`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/registry-inl.h#L86-L90) | 查找未命中时返回 `nullptr`            | 共享指针的空值语义清晰，调用方可直接判断 |
| mold   | [`src/mold.h#L272-L277`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/mold.h#L272-L277)                                                     | 用 `nullptr` 初始化可选输出节指针成员 | 成员默认空值表示尚未建立映射             |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖默认参数、查找结果与成员初始化三种用途。

## 陷阱与边界

**不要再用 `NULL`。** `NULL` 的具体类型由实现决定，可能是 `0`、`0L` 或 `__null`；重载和模板推导都可能出错。新代码统一写 `nullptr`。

**`0` 仍是空指针常量。** 为了兼容旧代码，`int* p = 0;` 合法；但这不意味着应该继续使用。重载场景下 `0` 会优先匹配 `int`。

**`nullptr` 不能转整数。** `reinterpret_cast<int>(nullptr)` 不合法；需要句柄数值时用 `reinterpret_cast<std::uintptr_t>(ptr)`，并明确处理空值。

**模板推导得到 `std::nullptr_t`。** `template <class T> void f(T); f(nullptr);` 中 `T` 是 `std::nullptr_t`，不是 `int*`；需要指针语义时显式写 `static_cast<int*>(nullptr)` 或调整重载。

**空指针不等于未定义值。** 未初始化的指针可能含任意地址；`nullptr` 只是明确的空值标记，不能靠它判断对象是否仍然有效。

**别用裸指针表达可选值。** 需要区分“没有值”和“有值”时，`std::optional` 或智能指针能表达所有权和生命周期，裸指针只表达地址。

## 库联动

- **智能指针**：`std::unique_ptr`、`std::shared_ptr` 的默认构造、`reset()` 与比较都使用 `nullptr`。
- **`std::function`**：空函数对象可直接与 `nullptr` 比较，赋值 `nullptr` 可清空。
- **C 接口**：向期望指针的 C API 传 `nullptr` 比 `NULL` 更明确，也不会触发整数重载。
- **drogon / spdlog / mold**：Web 框架、日志库与链接器都用 `nullptr` 表达缺省参数、查找失败与未初始化成员。

## 横向对比

| 方案            | 类型             | 可转整数 | 重载决议       | 主要限制                     |
| --------------- | ---------------- | -------- | -------------- | ---------------------------- |
| `NULL`          | 实现定义的整数宏 | 是       | 可能选错       | 类型不明确，模板推导错误     |
| `0`             | `int`            | 是       | 优先匹配整数   | 语义模糊，易误用             |
| `nullptr`       | `std::nullptr_t` | 否       | 精确匹配空指针 | 不能直接当整数使用           |
| `std::optional` | 可选值包装       | 不适用   | 不适用         | 有存储与访问开销，语义更丰富 |

跨标准演进：C++11 通过 N2431 引入 `nullptr` 与 `std::nullptr_t`；此后 `NULL` 仍保留兼容，但新代码应统一使用 `nullptr`。
