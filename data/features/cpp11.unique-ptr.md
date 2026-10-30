---
id: cpp11.unique-ptr
title: std::unique_ptr
standard: C++11
kind: library
category: [所有权, 内存管理]
difficulty: 2
status: final
tier: B
compilers: {gcc: "4.4", clang: "3.0", msvc: "18.0"}
tags: [unique_ptr, 独占所有权, RAII, 移动语义, 自定义删除器]
alternatives: [裸指针加 new/delete, shared_ptr, 值语义, 自定义 RAII 包装]
proposal: N1856
last_verified: "2026-10-30"
claims:
  - id: c1
    statement: "用 unique_ptr 成员表达输入段与重定位段的独占所有权"
    evidence: [ev-mold-unique-ptr-sections-001]
  - id: c2
    statement: "用 unique_ptr 参数转移格式化器的所有权"
    evidence: [ev-spdlog-unique-ptr-formatter-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

裸指针加 `new`/`delete` 把释放责任留给调用方：提前返回、异常抛出或路径分支都会漏掉 `delete`；复制指针又会让两个所有者指向同一对象，最终重复释放。`std::auto_ptr` 曾尝试表达所有权，但它在复制时偷偷转移所有权，无法放入标准容器，已被废弃。

`std::unique_ptr` 用类型表达“唯一所有者”：对象析构时自动释放，复制被禁用，所有权只能通过移动转移。它没有额外开销——默认删除器是空基类，指针大小的对象即可完成管理。标准提案为 N1856。

## 基础用法

用 `std::unique_ptr<T>` 持有对象，用 `std::move` 转移所有权。C++11 没有 `std::make_unique`，通常先 `new` 再交给构造函数。

<!-- compile -->

```cpp
#include <iostream>
#include <memory>
#include <utility>

struct Resource {
  explicit Resource(int id) : id(id) {}
  ~Resource() { std::cout << "release " << id << '\n'; }
  int id;
};

std::unique_ptr<Resource> make_resource(int id) {
  return std::unique_ptr<Resource>(new Resource(id));
}

int main() {
  std::unique_ptr<Resource> first = make_resource(7);
  std::unique_ptr<Resource> second = std::move(first);
  std::cout << (first ? "owned" : "moved") << ' ' << second->id << '\n';
}
```

输出 `moved 7`，随后在 `main` 结束时打印 `release 7`。`second` 接管后 `first` 变为空，释放只发生一次。

## 真实项目证据

| 项目   | 位置                                                                                                                                                        | 用法                                           | 为什么典型                               |
| ------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------- | ---------------------------------------- |
| mold   | [`src/mold.h#L533-L535`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/mold.h#L533-L535)                                 | 用 `unique_ptr` 容器成员与单个成员持有段对象   | 所有权随父对象析构，禁止复制避免重复释放 |
| spdlog | [`include/spdlog/spdlog-inl.h#L23-L30`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/spdlog-inl.h#L23-L30) | `set_formatter` 接收 `unique_ptr` 并移入注册表 | API 用参数类型明确表达所有权转移         |

## 陷阱与边界

**同一裸指针不能构造两个 `unique_ptr`。** `std::unique_ptr<T> a(p); std::unique_ptr<T> b(p);` 会让两者都尝试释放 `p`。指针已经归某个 `unique_ptr` 所有后，只能通过 `get()` 观察，不能再交给另一个所有者。

**传参方式表达所有权。** 函数要接管对象就按值接收 `std::unique_ptr<T>` 并 `std::move` 传入；只借用则接收 `T*` 或 `T&`，不要让调用方为了传参而无谓转移。

**删除器会改变类型。** `std::unique_ptr<T, D>` 的 `D` 参与类型，不同删除器的 `unique_ptr` 不能互相赋值；函数指针删除器还会让对象不再是零开销。

**数组要用 `unique_ptr<T[]>`。** 普通 `unique_ptr<T>` 对数组调用 `delete` 而不是 `delete[]`；C++11 提供数组特化，但更推荐 `std::vector` 或 `std::array`。

**`std::make_unique` 是 C++14。** C++11 代码里只能写 `std::unique_ptr<T>(new T(...))`；自己封装时要注意异常安全，避免 `new T(f(), g())` 中参数求值顺序带来的泄漏。

**不要用 `get()` 接管生命周期。** `release()` 交出裸指针后必须由调用方负责释放；混用 `get()` 与手工 `delete` 会产生悬空指针和二次释放。

## 横向对比

| 方案              | 所有权表达 | 复制   | 运行期开销     | 主要限制                   |
| ----------------- | ---------- | ------ | -------------- | -------------------------- |
| `unique_ptr`      | 独占       | 禁止   | 默认零额外开销 | 不能共享，转移需显式移动   |
| `shared_ptr`      | 共享       | 允许   | 原子引用计数   | 有循环引用与控制块成本     |
| 裸指针 + 手工管理 | 无         | 允许   | 无             | 泄漏、重复释放、异常不安全 |
| 值语义            | 独占       | 值复制 | 取决于类型     | 大对象复制昂贵，不适合多态 |

跨标准演进：C++11 通过 N1856 引入 `unique_ptr` 并废弃 `auto_ptr`；C++14 增加 `std::make_unique`；C++17 起 `auto_ptr` 从标准中移除。
