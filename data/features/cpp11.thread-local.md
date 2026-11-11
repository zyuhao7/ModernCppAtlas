---
id: cpp11.thread-local
title: 线程局部存储（thread_local）
standard: C++11
kind: language
category: [并发, 语言机制]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.8", clang: "3.3", msvc: "19.0"}
tags: [thread_local, TLS, 线程局部, 每线程状态]
alternatives: [全局变量加锁, pthread_key, __thread, 显式传参, 协程上下文]
proposal: N2659
last_verified: "2026-11-07"
claims:
  - id: c1
    statement: "用 thread_local 保存每线程的 MDC 上下文映射"
    evidence: [ev-spdlog-thread-local-mdc-001]
  - id: c2
    statement: "用 thread_local 缓冲区复用 __cxa_demangle 的输出内存"
    evidence: [ev-mold-thread-local-demangle-002]
  - id: c3
    statement: "用 thread_local 保存每线程的 MySQL 线程环境"
    evidence: [ev-drogon-thread-local-mysql-env-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

有些状态天然属于当前线程：日志上下文、错误码、临时缓冲区、数据库连接环境。用全局变量保存会被所有线程共享，必须加锁且容易串数据；用函数参数层层传递又会污染接口；平台扩展如 `__thread`、`pthread_key_create` 不可移植，且对非平凡类型支持有限。

`thread_local` 给变量线程存储期（thread storage duration）：每个线程拥有独立实例，初始化按线程进行，析构在线程退出时自动完成。它可以修饰命名空间作用域、块作用域和静态数据成员，非平凡类型也享受正常的构造与析构。标准提案为 N2659。

## 基础用法

`thread_local` 变量在每个线程第一次使用前初始化；`std::thread` 启动的新线程拿到自己的副本，主线程也有一份。块作用域的 `static thread_local` 同样是每线程惰性初始化的。

<!-- compile -->

```cpp
#include <iostream>
#include <string>
#include <thread>

thread_local std::string trace;

void note(const std::string& value) {
  trace += value;
  std::cout << trace << '\n';
}

int main() {
  std::thread worker([] {
    note("w");
    note("x");
  });
  worker.join();
  note("m");
}
```

输出三行 `w`、`wx`、`m`。工作线程的 `trace` 从空串开始累加，`join` 之后主线程访问的是自己的实例，因此只看到 `m`。完整版本见 [`examples/cpp11/thread_local.cpp`](../../examples/cpp11/thread_local.cpp)，四个场景的输出为：

```text
[1] w
[1] wx
[1] m
[2] worker ids = 1 2
[2] main ids = 1 2
[3] session closed for worker
[4] per_thread = 1 1 1, shared = 3
```

场景 1 是命名空间作用域的实例隔离：工作线程看到 `w`、`wx`，主线程的 `trace` 仍是空的，所以只打印 `m`。场景 2 用块作用域的 `static thread_local` 计数，工作线程与主线程都从 0 开始，各自得到 `1 2`。场景 3 证明非平凡类型由线程自己负责析构：`Session` 的析构函数在工作线程退出时执行。场景 4 对照普通 `static` 与 `thread_local`：`shared` 跨三个线程累加到 3，`per_thread` 在每个新线程里都重置为 1。

## 进阶用法

**块作用域惰性初始化。** 函数内 `static thread_local` 在所在线程首次执行到声明时构造，线程结束才析构；适合按线程缓存昂贵对象，例如缓冲区或解析器实例。

**与 `static`、`extern` 组合。** `thread_local` 控制存储期，`static` 控制链接；命名空间作用域的 `thread_local` 默认外部链接，加 `static` 可限制在本翻译单元。

**销毁顺序遵循构造逆序。** 同一线程内的 `thread_local` 对象在退出时按构造完成顺序的逆序析构；析构函数不应再依赖已销毁的其他线程局部对象。

**`constinit` 避免动态初始化。** C++20 允许 `constinit thread_local`，把初始化提前到编译期，减少每线程首次访问的守卫开销。

**注意线程池的复用。** 线程池中的工作线程长期存活，`thread_local` 状态会跨任务保留；任务之间必须显式重置，否则会“继承”上一个任务的数据。

**性能不是零成本。** 访问线程局部变量通常比普通全局变量多一次 TLS 寻址，动态初始化路径还有每线程的守卫检查；热路径上应测量而不是假设。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                             | 用法                         | 为什么典型                                           |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ---------------------------- | ---------------------------------------------------- |
| spdlog | [`include/spdlog/mdc.h#L44-L49`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/mdc.h#L44-L49)                                                    | 每线程 MDC 上下文映射        | 日志上下文必须按线程隔离，读路径无需加锁             |
| mold   | [`lib/demangle.cc#L13-L26`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/demangle.cc#L13-L26)                                                                | 缓存解符号缓冲区，跨调用复用 | 高频解符号避免重复分配，每线程独立缓冲区无竞争       |
| drogon | [`orm_lib/src/mysql_impl/MysqlConnection.cc#L51-L60`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/src/mysql_impl/MysqlConnection.cc#L51-L60) | 每线程 MySQL 环境对象        | MySQL 线程相关状态要求按线程初始化，事件循环各自持有 |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖日志上下文、解符号缓冲和数据库环境三种生命周期。

## 陷阱与边界

**它不提供同步。** 每个线程看到的是不同对象，`thread_local` 不能替代原子变量或互斥量；需要跨线程可见的状态仍要显式共享并同步。

**地址不能跨线程传递。** 把一个线程的 `thread_local` 地址交给另一个线程会指向错误实例；需要共享时应当复制值或改用普通对象。

**线程退出即销毁。** 线程结束时 `thread_local` 的析构函数运行，其他线程若仍持有指针就会悬空；跨线程的生命周期必须由调用方另行保证。

**线程池中状态会累积。** 工作线程不退出，`thread_local` 对象不会重置；请求级上下文应在任务开始时清空，避免信息泄漏到下一个任务。

**动态库中的 TLS 有平台差异。** 某些平台对动态加载模块的线程局部变量支持受限，可能退化为 `emutls`；插件架构要验证目标平台的初始化与析构行为。

**构造可能抛出异常。** 非平凡 `thread_local` 的初始化可以抛异常，首次使用点需要能处理失败；构造失败后该实例不会被视为已初始化。

**大对象会放大内存。** 每个线程一份实例，线程数乘以对象大小就是实际占用；大缓冲区更适合放在线程池的显式工作区而不是 `thread_local`。

## 库联动

- **`<thread>`**：`std::thread` 与 `thread_local` 配合，新线程自动获得独立实例，无需额外注册或清理。
- **`<mutex>`**：线程局部状态本身无需加锁，但与其他线程共享的数据仍要互斥量或原子操作。
- **`<string>` / `<vector>`**：非平凡容器可以直接作为 `thread_local`，构造和析构按线程自动完成。
- **spdlog**：MDC 用 `thread_local` 保存日志键值，日志语句无需传上下文参数。
- **数据库客户端**：MySQL、PostgreSQL 等库的线程相关句柄常放在 `thread_local` 中，drogon 的证据是典型做法。

## 横向对比

| 方案           | 每线程实例 | 非平凡类型 | 可移植性   | 主要代价                     |
| -------------- | ---------- | ---------- | ---------- | ---------------------------- |
| `thread_local` | 是         | 支持       | 标准       | TLS 寻址与每线程初始化开销   |
| 全局变量加锁   | 否         | 支持       | 标准       | 锁竞争，状态容易串线程       |
| `pthread_key`  | 是         | 需手工析构 | POSIX      | 样板代码多，错误处理繁琐     |
| `__thread`     | 是         | 受限       | 编译器扩展 | 非平凡类型支持差，不可移植   |
| 显式传参       | 否         | 支持       | 标准       | 污染接口，跨越多层调用时繁琐 |

跨标准演进：C++11 通过 N2659 引入 `thread_local`；C++20 的 `constinit` 让线程局部变量可以静态初始化；后续标准没有改变存储期语义。协程的兴起让“当前执行上下文”有了新的表达方式，但线程池场景下 `thread_local` 仍是每线程状态的标准载体。
