---
id: cpp11.thread
title: std::thread
standard: C++11
kind: library
category: [并发]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "16.0"}
tags: [std::thread, 并发, join, detach, hardware_concurrency]
alternatives: [std::jthread, pthread, std::async, 线程池, 协程]
proposal: N2497
last_verified: "2026-11-03"
claims:
  - id: c1
    statement: "用 std::thread 在构造函数里启动事件循环，并在析构函数里 join"
    evidence: [ev-stdexec-thread-single-thread-context-001]
  - id: c2
    statement: "用 std::thread 运行阻塞的 waitpid 循环，避免阻塞启动线程"
    evidence: [ev-hyprland-thread-waitpid-002]
  - id: c3
    statement: "用 std::thread::hardware_concurrency 限制数据库事件循环的线程数"
    evidence: [ev-drogon-thread-hardware-concurrency-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 之前，标准库没有线程抽象，跨平台代码只能在 `pthread`、Win32 线程和各自的条件编译之间切换。线程入口必须是固定签名的函数指针，参数通过 `void*` 传递，错误处理依赖返回值；线程句柄的创建、等待和销毁也没有统一的 RAII 语义。

`std::thread` 把“启动一个可调用对象作为新线程”变成标准库操作：入口可以是函数、lambda 或带状态的函数对象，参数按值转发给可调用对象，`join()` 等待结束，`detach()` 放弃所有权。标准提案为 N2497，头文件是 `<thread>`。

## 基础用法

构造 `std::thread` 时传入可调用对象和参数；对象构造完成即代表新线程已经开始执行。线程对象是移动-only 的，只能拥有一个执行体。析构时如果仍处于 joinable 状态，程序会调用 `std::terminate`，因此每条路径都必须 `join()` 或 `detach()`。

<!-- compile -->

```cpp
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

void accumulate(int from, int to, long long& out) {
  long long sum = 0;
  for (int value = from; value <= to; ++value) {
    sum += value;
  }
  out = sum;
}

int main() {
  const int workers = 4;
  const int per_worker = 25;
  std::vector<long long> partial(workers, 0);
  std::vector<std::thread> threads;
  threads.reserve(workers);

  for (int i = 0; i < workers; ++i) {
    const int from = i * per_worker + 1;
    const int to = from + per_worker - 1;
    threads.emplace_back(accumulate, from, to, std::ref(partial[i]));
  }

  std::cout << threads.size() << ' ' << (threads.front().joinable() ? 1 : 0)
            << '\n';

  for (auto& worker : threads) {
    worker.join();
  }

  long long total = 0;
  for (long long value : partial) {
    total += value;
  }

  std::cout << partial.front() << ' ' << partial.back() << ' ' << total << '\n';

  long long doubled = 0;
  std::thread lambda_worker([&doubled] { doubled = 21 * 2; });
  lambda_worker.join();
  std::cout << doubled << '\n';
}
```

输出 `4 1`、`325 2200 5050` 与 `42`。四个线程分别累加 1–25、26–50、51–75、76–100，主线程在 `join()` 后读取各自结果再汇总；`std::ref` 让线程拿到 `partial[i]` 的引用而不是副本。第二个线程演示 lambda 直接捕获局部变量。完整版本见 [`examples/cpp11/thread.cpp`](../../examples/cpp11/thread.cpp)。

参数默认按值复制到线程的存储中；需要共享对象时用 `std::ref`/`std::cref` 显式包装。`join()` 只能调用一次，调用后线程变为 non-joinable；`detach()` 让线程在后台继续运行，线程对象不再能等待它。

## 进阶用法

线程入口的可调用对象在构造时就被复制或移动，因此捕获的 `unique_ptr` 等移动-only 对象要配合 `std::move`。参数也是按值转发：传引用必须用 `std::ref`，否则修改的是线程内部的副本。

线程数量应当受控。`std::thread::hardware_concurrency()` 返回实现建议的并发线程数，但允许返回 0；它只是提示，不是可用核数的保证。数据库连接池、线程池常用它与任务数取较小值来定容。

`thread_local` 变量为每个线程保存独立实例，适合缓存和错误状态；但它的构造与析构时机由线程生命周期决定，长时间驻留的线程要避免无限增长的线程局部容器。

需要“可中断、可自动 join”的线程时，C++20 的 `std::jthread` 更合适：析构时自动请求停止并 join，还带 `stop_token`。只需要一次性异步结果时，`std::async` 配合 `std::future` 能省去手工管理线程对象。

## 真实项目证据

| 项目     | 位置                                                                                                                                                                               | 用法                                                     | 为什么典型                                         |
| -------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------- | -------------------------------------------------- |
| stdexec  | [`include/exec/single_thread_context.hpp#L27-L40`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/single_thread_context.hpp#L27-L40) | 构造函数里用 lambda 启动 `run_loop`，析构函数里 `join()` | 把线程生命周期绑定到拥有者，构造即启动、析构即汇合 |
| Hyprland | [`start/src/core/Instance.cpp#L76-L87`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/start/src/core/Instance.cpp#L76-L87)                      | 用独立线程执行阻塞的 `waitpid` 循环                      | 阻塞系统调用移出主线程，避免卡住启动流程           |
| drogon   | [`orm_lib/src/DbClientImpl.cc#L66-L71`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/src/DbClientImpl.cc#L66-L71)               | 用 `hardware_concurrency()` 给数据库事件循环线程数封顶   | 线程池定容时把硬件并发度作为上限                   |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖线程封装、阻塞调用卸载与线程池定容三种典型形态。

## 陷阱与边界

**析构时仍 joinable 会终止进程。** `std::thread` 的析构函数不会隐式 join，也不会隐式 detach；忘记 `join()` 或 `detach()` 会调用 `std::terminate`。

**不能 join 自己。** 在目标线程内部对自己的 `std::thread` 对象调用 `join()` 会抛 `std::system_error`，常见于把线程对象放进共享容器后误用。

**detach 后的生命周期完全由调用方负责。** 分离线程可能访问已经销毁的局部变量或成员；除非线程真正独立，否则优先 join。

**共享数据仍需自己同步。** `std::thread` 只负责创建执行流，不提供数据保护。多个线程同时写同一对象、同时扩容同一容器，都是数据竞争。

**线程函数抛出的异常会终止进程。** 异常不能穿过线程边界传播；需要在线程内部捕获并转成结果、错误码或 `std::exception_ptr`。

**`hardware_concurrency()` 可能返回 0。** 标准允许实现无法检测时返回 0，直接拿它做除数或容器大小会出问题。

**线程不是免费的。** 每个线程有独立的栈和内核调度开销；大量短任务应该交给线程池，而不是为每个任务新建线程。

## 库联动

- **`std::jthread`**：C++20 的自动 join 线程，析构时请求停止并等待，替代大多数裸 `std::thread` 成员。
- **`std::mutex` / `std::condition_variable`**：线程之间传递数据的基本同步原语；`std::thread` 不替你做互斥。
- **`std::atomic`**：无锁计数器和标志位；比互斥量更适合简单共享状态。
- **`std::async` / `std::future`**：一次性异步任务，省去手工 join 和结果传递。
- **线程池与执行器**：stdexec、TBB 等提供任务调度；`std::thread` 更多作为底层执行单元。

## 横向对比

| 方案           | 标准版本 | 生命周期管理     | 返回值传递       | 主要代价                         |
| -------------- | -------- | ---------------- | ---------------- | -------------------------------- |
| `std::thread`  | C++11    | 手动 join/detach | 需共享变量或包装 | 忘记汇合会终止进程               |
| `std::jthread` | C++20    | 析构自动 join    | 仍需共享变量     | 需要支持 stop_token 的协作式取消 |
| `std::async`   | C++11    | future 管理      | `std::future`    | 启动策略由实现决定               |
| `pthread`      | 平台     | 手动 join/detach | `void*` 参数     | 不可移植，缺少类型安全           |
| 线程池         | 第三方   | 池统一管理       | 任务返回值       | 引入依赖，调优空间与实现绑定     |

跨标准演进：C++11 通过 N2497 引入 `std::thread` 与整套线程同步设施；C++20 增加 `std::jthread` 和停止令牌，把协作式取消与自动汇合纳入标准；截至 C++23，线程创建本身仍不提供跨平台的优先级或亲和性接口。
