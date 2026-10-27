---
id: cpp14.make-unique
title: std::make_unique
standard: C++14
kind: library
category: [内存管理, 所有权]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.9", clang: "3.4", msvc: "18.0"}
tags: [make_unique, unique_ptr, 所有权, RAII, 工厂]
alternatives: [std::unique_ptr(new T), std::make_shared, 裸 new/delete, 值语义]
proposal: N3656
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用 std::make_unique 在工厂代码中创建派生段对象"
    evidence: [ev-mold-make-unique-section-001]
  - id: c2
    statement: "用 std::make_unique 创建由框架唯一持有的线程池成员"
    evidence: [ev-drogon-make-unique-thread-pool-002]
  - id: c3
    statement: "用 std::make_unique 从工厂函数返回多态引擎"
    evidence: [ev-abseil-make-unique-engine-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

`std::unique_ptr` 解决了所有权问题，但它的构造函数仍然要求显式 `new`：`std::unique_ptr<T>(new T(args...))`。类型名写了两遍，`new` 与智能指针的构造分散在同一个表达式里。更麻烦的是函数实参求值顺序：`f(std::unique_ptr<T>(new T), g())` 中如果 `g()` 先执行并抛出异常，裸指针就泄漏了。

`std::make_unique` 把分配和所有权包装成一个函数调用，类型只写一次，构造参数直接转发。它在 C++14 通过 N3656 进入标准库，补上了 C++11 只有 `std::make_shared` 的空缺。

## 基础用法

`std::make_unique<T>(args...)` 在堆上构造 `T`，返回管理它的 `std::unique_ptr<T>`。参数按完美转发传给构造函数。

<!-- compile -->

```cpp
#include <iostream>
#include <memory>
#include <string>
#include <utility>

struct Session {
  explicit Session(std::string name) : name(std::move(name)) {}

  std::string name;
};

int main() {
  auto session = std::make_unique<Session>("alpha");
  auto value = std::make_unique<int>(42);
  std::cout << session->name << ' ' << *value << '\n';
}
```

输出 `alpha 42`。`session` 和 `value` 都是 `std::unique_ptr`，离开作用域时自动删除对象。完整版本见 [`examples/cpp14/make_unique.cpp`](../../examples/cpp14/make_unique.cpp)。

数组也有对应重载：`std::make_unique<int[]>(n)` 创建 `n` 个元素并值初始化，返回 `std::unique_ptr<int[]>`。

## 进阶用法

工厂函数是最常见的使用场景。返回 `std::unique_ptr<Base>` 时，用 `std::make_unique<Derived>(args...)` 构造具体类型，再由 `unique_ptr` 的转换构造转成基类指针；不要写 `std::make_unique<Base>(args...)`，那会尝试直接构造抽象基类。

把 `unique_ptr` 存进容器或成员时，`make_unique` 让“分配 + 转移所有权”成为一个表达式。容器扩容只移动指针，不复制对象；成员初始化列表里也能直接写 `member_(std::make_unique<T>(...))`。

需要自定义删除器时，`make_unique` 帮不上忙。删除器是 `unique_ptr` 类型的一部分，必须显式构造：`std::unique_ptr<FILE, decltype(&fclose)> file(fopen(...), &fclose);`。需要共享所有权时改用 `std::make_shared`，它把控制块和对象放在一次分配里。

C++20 增加 `std::make_unique_for_overwrite`，用于“分配后立即覆盖、不需要值初始化”的场景。普通 `make_unique<T[]>(n)` 会对每个元素值初始化，处理大块缓冲区时这一步可能不便宜。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                      | 用法                                        | 为什么典型                       |
| ------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------- | -------------------------------- |
| mold   | [`src/input-files.cc#L371-L374`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/input-files.cc#L371-L374)                                               | 在数组槽位中用 `make_unique` 创建派生段对象 | 工厂式构造，所有权立即交给容器   |
| drogon | [`lib/src/HttpAppFrameworkImpl.cc#L612-L614`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/src/HttpAppFrameworkImpl.cc#L612-L614)          | 创建只由框架持有的线程池成员                | 分配与赋值合并为单个表达式       |
| abseil | [`absl/crc/internal/crc_memcpy_fallback.cc#L68-L71`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/crc/internal/crc_memcpy_fallback.cc#L68-L71) | 从工厂函数返回多态引擎                      | 返回 `unique_ptr` 表达唯一所有权 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，分别对应容器槽位、长期成员和工厂返回值。

## 陷阱与边界

**不支持自定义删除器。** 需要 `fclose`、`munmap` 等删除动作时，必须显式构造带删除器类型的 `unique_ptr`。

**花括号初始化列表不能直接推导。** `std::make_unique<std::vector<int>>({1, 2, 3})` 无法推导 `Args`，应写成 `std::make_unique<std::vector<int>>(std::initializer_list<int>{1, 2, 3})`。

**数组重载会值初始化。** `std::make_unique<int[]>(n)` 把每个元素置零；只想要未初始化缓冲区时，C++20 起用 `make_unique_for_overwrite`。

**不要先 `new` 再交给 `make_unique`。** `std::make_unique<T>(*new T)` 会先分配再复制，既泄漏又失去意义。

**多态构造要写具体类型。** `make_unique<Base>` 只会构造 `Base`；需要派生行为时写 `make_unique<Derived>`，再让返回值转换成基类指针。

**异常安全的前提是一次表达式。** 只要分配和所有权转移在同一个完整表达式里，后续实参抛异常也不会泄漏；把裸指针先存进局部变量再包装，就又回到了手动管理。

## 库联动

- **`std::unique_ptr`**：`make_unique` 是它的首选工厂，二者共同表达唯一所有权。
- **标准容器**：`std::vector<std::unique_ptr<T>>` 存放多态对象，扩容时只移动指针。
- **工厂与插件系统**：返回 `unique_ptr<Interface>` 隐藏具体类型，调用方无需知道派生类。
- **mold / drogon / abseil**：链接器、Web 框架和 CRC 引擎都用它把分配点集中在一处。

## 横向对比

| 方案                        | 类型名重复 | 异常安全 | 自定义删除器 | 主要限制                       |
| --------------------------- | ---------- | -------- | ------------ | ------------------------------ |
| `std::unique_ptr<T>(new T)` | 两次       | 需小心   | 支持         | 实参求值顺序可能泄漏           |
| `std::make_unique<T>`       | 一次       | 是       | 不支持       | 花括号列表需显式类型           |
| `std::make_shared<T>`       | 一次       | 是       | 不支持       | 共享所有权，控制块与对象同分配 |
| 裸 `new` / `delete`         | 两次       | 否       | 手动         | 容易忘记释放或重复释放         |

跨标准演进：C++11 提供 `unique_ptr` 与 `make_shared`；C++14 通过 N3656 补上 `make_unique`；C++20 增加 `make_unique_for_overwrite`，避免不必要的值初始化。
