---
id: cpp11.call-once
title: 一次性初始化（std::call_once）
standard: C++11
kind: library
category: [并发, 初始化]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "19.0"}
tags: [一次性初始化, once_flag, 线程安全, 懒初始化]
alternatives: [函数内静态变量, 双检锁, std::atomic 标志, 启动时显式初始化]
proposal: N2320
last_verified: "2026-11-10"
claims:
  - id: c1
    statement: "静态 once_flag 只配置一次 JSON 解析器"
    evidence: [ev-drogon-call-once-json-config-001]
  - id: c2
    statement: "类静态 once_flag 只设置一次 SQLite 全局配置"
    evidence: [ev-drogon-call-once-sqlite-config-002]
  - id: c3
    statement: "call_once 惰性编译 glob 模式"
    evidence: [ev-mold-call-once-glob-compile-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

多线程下的懒初始化只要写错一处就会出问题：普通布尔标志存在数据竞争，双检锁需要正确的内存序，手写 `std::atomic` 标志又容易漏掉“初始化完成后才发布结果”的同步。

```cpp
if (!ready) {          // 两个线程可能同时进入
  build();
  ready = true;        // 其他线程可能看见 ready 却看见半初始化对象
}
```

C++11 在 `<mutex>` 中提供 `std::once_flag` 与 `std::call_once`：所有并发调用者中恰好一个执行初始化函数，其余调用者阻塞到它完成，并与之同步。标准提案为 N2320。

## 基础用法

把初始化逻辑交给 `std::call_once`，并发调用也只会执行一次。

<!-- compile -->

```cpp
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

int main() {
  std::once_flag flag;
  int initialized = 0;
  auto init = [&initialized] { ++initialized; };

  std::vector<std::thread> workers;
  for (int i = 0; i < 8; ++i) {
    workers.emplace_back([&] { std::call_once(flag, init); });
  }
  for (auto& worker : workers) {
    worker.join();
  }

  std::cout << initialized << '\n';
}
```

输出 `1`。八个线程同时调用 `std::call_once`，`init` 只执行一次，其余线程等待它完成后再继续。完整版本见 [`examples/cpp11/call_once.cpp`](../../examples/cpp11/call_once.cpp)。

## 进阶用法

**`once_flag` 不能拷贝或移动。** 它通常声明为函数内局部静态变量或类静态成员；按值传递或放进容器都无法编译。

**初始化可以带参数。** `std::call_once(flag, &Config::load, &config, path)` 会把参数转发给可调用对象；需要引用语义时用 `std::ref` 包裹，否则参数按值复制。

**异常不会标记完成。** 若初始化函数抛出异常，异常传播给当前调用者，标志不视为已完成；后续调用者会重新尝试执行。

**与函数内静态变量分工不同。** C++11 起函数内静态变量的初始化本身线程安全；当初始化逻辑需要独立于对象生命周期、或需要在失败后重试时，`std::call_once` 更合适。

**同步语义是完整的。** 初始化函数完成与所有调用者返回之间建立 happens-before，初始化写入的数据对后续读取可见，不需要额外的原子操作或内存栅栏。

**可调用对象形式灵活。** 普通函数、lambda、`std::bind` 结果、成员函数指针都可以传入，参数在调用时转发。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                                       | 用法                                      | 为什么典型                                         |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------- | -------------------------------------------------- |
| drogon | [`lib/src/JsonConfigAdapter.cc#L10-L12`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/src/JsonConfigAdapter.cc#L10-L12)                                     | 静态 `once_flag` 只配置一次 JSON 解析器   | 多个请求线程并发进入，初始化仍只执行一次           |
| drogon | [`orm_lib/src/sqlite3_impl/Sqlite3Connection.cc#L97-L103`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/src/sqlite3_impl/Sqlite3Connection.cc#L97-L103) | 类静态 `once_flag` 只设置一次 SQLite 配置 | 连接可以创建多个实例，全局配置必须进程内只执行一次 |
| mold   | [`lib/multi-glob.cc#L27-L29`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/multi-glob.cc#L27-L29)                                                                      | `call_once` 惰性编译 glob 模式            | 首次匹配时才编译，后续调用直接复用结果             |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖函数内静态标志、类静态标志与成员标志三种生命周期。

## 陷阱与边界

**同一标志重入会死锁。** 初始化函数内部再调用 `std::call_once` 且传入同一个 `once_flag`，当前线程等待自己完成，行为未定义或直接死锁。

**异常会触发重试。** 初始化失败后标志保持未完成状态，后续调用者会再次执行；如果失败是永久性的，应当在初始化函数内部记录状态而不是反复抛异常。

**标志的生命周期必须覆盖所有调用。** `once_flag` 先于调用者销毁会导致未定义行为；函数内静态变量或类静态成员天然满足，局部自动变量在线程逃逸时会失效。

**不要把 `once_flag` 当锁用。** 它只保证一次性执行，不提供互斥区间；需要保护反复读写的共享数据应使用 `std::mutex`。

**参数按值转发。** `std::call_once` 的参数以 `Args&&` 接收但内部会存储副本，传入引用必须显式 `std::ref`，否则修改的是副本。

**MSVC 支持较晚。** `std::call_once` 要到 VS2015（19.0）才完整可用；更老的工具链只能退回函数内静态变量或手写同步。

## 库联动

- **`<mutex>`**：`std::once_flag` 与 `std::call_once` 定义在此头文件，和 `std::mutex`、`std::lock_guard` 同属同步设施。
- **`<thread>`**：并发调用者是线程，示例与线程创建、`join` 配套使用。
- **函数内静态变量**：C++11 保证其初始化线程安全，是最常见的 `call_once` 替代方案。
- **单例与全局配置**：懒汉单例、日志后端、数据库驱动全局设置常以 `call_once` 完成首次初始化。
- **`std::shared_ptr`**：需要延迟构造共享对象时，可在 `call_once` 内完成 `make_shared`。
- **drogon / mold**：分别展示配置初始化与惰性编译两类生产用法。

## 横向对比

| 方案               | 线程安全   | 失败重试   | 主要限制                                 |
| ------------------ | ---------- | ---------- | ---------------------------------------- |
| `std::call_once`   | 是         | 支持       | 标志生命周期与重入需要小心               |
| 函数内静态变量     | 是         | 不支持     | 只能随变量首次使用初始化，异常后无法重试 |
| 双检锁             | 取决于实现 | 支持       | 需要正确的原子操作与内存序               |
| `std::atomic` 标志 | 是         | 需自行实现 | 容易漏掉初始化结果的发布同步             |

跨标准演进：C++11 通过 N2320 引入线程库与 `std::call_once`，同时用 N2660 保证函数内静态变量初始化的线程安全；C++17 未改变两者语义；C++20 的 `std::latch`、`std::barrier` 补充了一次性同步的其他形态，但 `std::call_once` 仍是最直接的单次初始化工具。
