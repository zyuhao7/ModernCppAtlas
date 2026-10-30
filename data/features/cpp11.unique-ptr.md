---
id: cpp11.unique-ptr
title: std::unique_ptr
standard: C++11
kind: library
category: [所有权, 内存管理]
difficulty: 2
status: final
tier: A
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
  - id: c3
    statement: "用 unique_ptr 成员独占时区实现并显式禁用复制"
    evidence: [ev-abseil-unique-ptr-zone-003]
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

struct Connection {
  explicit Connection(int id) : id(id) {}
  ~Connection() { std::cout << "close " << id << '\n'; }
  int id;
};

std::unique_ptr<Connection> connect(int id) {
  return std::unique_ptr<Connection>(new Connection(id));
}

int main() {
  std::unique_ptr<Connection> primary = connect(1);
  std::unique_ptr<Connection> backup = connect(2);
  primary = std::move(backup);
  std::cout << (backup ? "held" : "moved") << '\n';
}
```

输出 `close 1`、`moved`，退出时再打印 `close 2`：赋值先把 `primary` 原来的连接释放，再接管 `backup` 的对象；`backup` 变为空，同一个对象只释放一次。完整版本见 [`examples/cpp11/unique_ptr.cpp`](../../examples/cpp11/unique_ptr.cpp)。

## 进阶用法

删除器是 `unique_ptr` 的第二个模板参数。声明 `std::unique_ptr<FILE, decltype(&fclose)>` 时删除器是函数指针，对象大小从 8 字节涨到 16 字节；无捕获的 lambda 或空结构体删除器能享受空基类优化，恢复零开销。删除器参与类型，因此 `unique_ptr<T, D1>` 与 `unique_ptr<T, D2>` 不能互相赋值。

所有权可以沿继承层次向上转换：`std::unique_ptr<Derived>` 能隐式移动构造 `std::unique_ptr<Base>`，前提是 `Base` 的析构函数为虚函数；反向转换不提供。`release()` 交出裸指针并清空自身，`reset()` 先释放旧对象再接管新指针，`get()` 只观察不移交。

工厂函数按值返回 `unique_ptr` 依赖移动语义，编译器会省略多余的移动。C++14 的 `std::make_unique` 把分配封装成一次调用，避免 `new T(f(), g())` 中参数求值顺序导致的泄漏；C++11 代码可以自己封装等价函数。

Pimpl 惯用法把实现细节放进 `std::unique_ptr<Impl>` 成员，头文件不再依赖实现类型的完整定义。代价是析构函数必须定义在实现文件里，否则编译器在实例化默认析构时看不到完整类型。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                              | 用法                                                  | 为什么典型                               |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------- | ---------------------------------------- |
| mold   | [`src/mold.h#L533-L535`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/mold.h#L533-L535)                                                                       | 用 `unique_ptr` 容器成员与单个成员持有段对象          | 所有权随父对象析构，禁止复制避免重复释放 |
| spdlog | [`include/spdlog/spdlog-inl.h#L23-L30`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/spdlog-inl.h#L23-L30)                                       | `set_formatter` 接收 `unique_ptr` 并移入注册表        | API 用参数类型明确表达所有权转移         |
| abseil | [`absl/time/internal/cctz/src/time_zone_impl.h#L80-L90`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/time/internal/cctz/src/time_zone_impl.h#L80-L90) | `unique_ptr<TimeZoneIf>` 成员持有实现，外层复制被删除 | 实现对象只被一处拥有，随父对象析构释放   |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖容器成员、API 参数与实现指针三种所有权形态。

## 陷阱与边界

**同一裸指针不能构造两个 `unique_ptr`。** `std::unique_ptr<T> a(p); std::unique_ptr<T> b(p);` 会让两者都尝试释放 `p`。指针已经归某个 `unique_ptr` 所有后，只能通过 `get()` 观察，不能再交给另一个所有者。

**传参方式表达所有权。** 函数要接管对象就按值接收 `std::unique_ptr<T>` 并 `std::move` 传入；只借用则接收 `T*` 或 `T&`，不要让调用方为了传参而无谓转移。

**删除器会改变类型。** `std::unique_ptr<T, D>` 的 `D` 参与类型，不同删除器的 `unique_ptr` 不能互相赋值；函数指针删除器还会让对象不再是零开销。

**数组要用 `unique_ptr<T[]>`。** 普通 `unique_ptr<T>` 对数组调用 `delete` 而不是 `delete[]`；C++11 提供数组特化，但更推荐 `std::vector` 或 `std::array`。

**`std::make_unique` 是 C++14。** C++11 代码里只能写 `std::unique_ptr<T>(new T(...))`；自己封装时要注意异常安全，避免 `new T(f(), g())` 中参数求值顺序带来的泄漏。

**不要用 `get()` 接管生命周期。** `release()` 交出裸指针后必须由调用方负责释放；混用 `get()` 与手工 `delete` 会产生悬空指针和二次释放。

**基类析构函数必须是虚函数。** 通过 `unique_ptr<Base>` 释放派生对象时，非虚析构只调用基类部分，派生部分泄漏；新增虚函数前先确认这一约定。

## 库联动

- **`<memory>`**：`std::default_delete` 是默认删除器；C++14 的 `std::make_unique` 提供异常安全的构造入口。
- **标准容器**：`std::vector<std::unique_ptr<T>>` 是异构多态容器的常见写法，元素只能移动；`std::map` 的 value 为 `unique_ptr` 时同样只支持移动插入。
- **`<cstdio>` / 系统资源**：`std::unique_ptr<FILE, decltype(&fclose)>` 把 C 接口纳入 RAII，socket、句柄的封装方式相同。
- **mold / spdlog**：链接器用 `unique_ptr` 容器管理输入段与重定位段，日志库用参数类型声明格式化器的所有权转移。
- **`std::shared_ptr`**：需要多个持有者时改用共享所有权，但要接受原子计数与循环引用问题。

## 横向对比

| 方案              | 所有权表达 | 复制   | 运行期开销     | 主要限制                   |
| ----------------- | ---------- | ------ | -------------- | -------------------------- |
| `unique_ptr`      | 独占       | 禁止   | 默认零额外开销 | 不能共享，转移需显式移动   |
| `shared_ptr`      | 共享       | 允许   | 原子引用计数   | 有循环引用与控制块成本     |
| 裸指针 + 手工管理 | 无         | 允许   | 无             | 泄漏、重复释放、异常不安全 |
| 值语义            | 独占       | 值复制 | 取决于类型     | 大对象复制昂贵，不适合多态 |

跨标准演进：C++11 通过 N1856 引入 `unique_ptr` 并废弃 `auto_ptr`；C++14 增加 `std::make_unique`；C++17 起 `auto_ptr` 从标准中移除。
