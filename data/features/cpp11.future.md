---
id: cpp11.future
title: 异步任务与 future（std::future / std::async）
standard: C++11
kind: library
category: [并发, 异步]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.5", clang: "3.0", msvc: "16.0"}
tags: [std::future, std::async, std::promise, 异步, 任务]
alternatives: [std::thread, 回调, 协程, 线程池, std::packaged_task]
proposal: N2996
last_verified: "2026-11-07"
claims:
  - id: c1
    statement: "用 std::async 在后台构造节点，并通过 wait_for 轮询状态"
    evidence: [ev-nano-future-async-node-001]
  - id: c2
    statement: "用 shared_ptr 持有 std::future 管理后台文件监视任务"
    evidence: [ev-glaze-file-watch-async-002]
  - id: c3
    statement: "用 promise 与 future 把协程结果桥接成旧式异步接口"
    evidence: [ev-drogon-co-future-bridge-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 之前，把工作放到另一个线程后取回结果是平台相关的：`pthread_create` 用 `void*` 传参，返回值靠调用方自己约定，异常无法跨线程传播；直接开线程又要求调用方手工管理 join 和错误分支。

`<future>` 把“启动工作”和“取回结果”拆成两个对象：任务侧写入共享状态，`std::future` 在另一侧等待并读取。`std::async` 是最简入口，直接返回 `future`；`std::promise` 适合回调式接口，`std::packaged_task` 把可调用对象包装成任务。异常会被存进共享状态，在 `get()` 时重新抛出。标准提案为 N2996。

## 基础用法

`std::async(std::launch::async, callable)` 启动异步任务并返回 `future`；`get()` 等待完成并取出结果，只能调用一次。`std::promise` 与 `future` 成对使用，`set_value` 之后等待方才能继续。

<!-- compile -->

```cpp
#include <chrono>
#include <future>
#include <iostream>
#include <numeric>
#include <vector>

int main() {
  std::future<int> sum = std::async(std::launch::async, [] {
    std::vector<int> values{1, 2, 3, 4};
    return std::accumulate(values.begin(), values.end(), 0);
  });
  std::cout << (sum.valid() ? "valid" : "empty") << ' ' << sum.get() << '\n';

  std::promise<int> promise;
  std::future<int> answer = promise.get_future();
  promise.set_value(42);
  std::cout << answer.get() << '\n';
}
```

输出 `valid 10` 与 `42`。`sum.valid()` 在 `get()` 之前为真，`promise` 写入后等待方立即就绪。完整版本见 [`examples/cpp11/future.cpp`](../../examples/cpp11/future.cpp)。

## 进阶用法

**选择启动策略。** `std::launch::async` 强制在新线程执行，`std::launch::deferred` 推迟到 `get()` 或 `wait()` 时才在当前线程调用。不传策略时实现可任选，因此“任务是否已经在跑”不可假设；需要立即并发就显式写 `async`。

**用 `wait_for` 保持响应。** GUI 或事件循环里不要直接 `get()` 阻塞，可以循环调用 `wait_for(0s)` 或 `wait_for(50ms)` 检查 `std::future_status`，把控制权还给事件循环。

**异常沿 future 传播。** 任务中抛出的异常会存进共享状态，`get()` 时重新抛出；`promise` 侧用 `set_exception(std::current_exception())` 传递异构错误。

**`std::packaged_task` 延迟提交。** 它把可调用对象和共享状态绑定，可以先构造，稍后交给线程池或事件循环执行；相比 `std::async` 更容易接入自定义调度器。

**多消费者用 `shared_future`。** `future` 是移动-only 的单消费者句柄；需要多个线程等待同一结果时调用 `share()` 得到 `std::shared_future`，每个副本都可以 `get()`。

**任务所有权影响析构。** `std::async` 返回的 `future` 在析构时会等待任务完成，这一隐式同步常被用作“发后不管”的清理手段；反过来，如果忘记保存返回值，临时 `future` 的析构会让调用点一直阻塞到任务结束。

## 真实项目证据

| 项目      | 位置                                                                                                                                                                               | 用法                                    | 为什么典型                                              |
| --------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------- | ------------------------------------------------------- |
| nano-node | [`nano/nano_wallet/entry.cpp#L206-L219`](https://github.com/nanocurrency/nano-node/blob/6b5f63ce04c9a1ffe37b8c687fffc1bef941bf98/nano/nano_wallet/entry.cpp#L206-L219)             | 后台构造节点，`wait_for` 轮询状态       | GUI 线程用事件循环等待，future 提供完成信号而不阻塞界面 |
| glaze     | [`include/glaze/file/file_watch.hpp#L29-L42`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/file/file_watch.hpp#L29-L42)       | `shared_ptr` 持有 `future` 管理监视任务 | 句柄销毁时 future 析构等待任务结束，天然完成 join       |
| drogon    | [`lib/inc/drogon/utils/coroutine.h#L569-L583`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/utils/coroutine.h#L569-L583) | promise/future 桥接协程与旧式异步接口   | 框架同时保留协程和回调，promise 是低成本兼容层          |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖 GUI 事件循环、生命周期管理和协程桥接三种形态。

## 陷阱与边界

**`std::async` 的 future 析构会阻塞。** 返回的 `future` 若仍持有异步任务，析构函数会等待任务完成；这既能自动清理，也可能在不经意处卡住调用方。需要真正“发后不管”时应把 `future` 交给长期对象或使用线程池。

**默认策略可能推迟执行。** 不写 `std::launch::async` 时，实现可以选择 `deferred`，任务直到 `get()` 才开始跑；依赖并发的代码必须显式指定策略。

**`get()` 只能调用一次。** 普通 `future` 在 `get()` 之后变为 invalid，再次调用是未定义行为；需要重复读取时使用 `shared_future`。

**`get()` 没有超时。** 它一直阻塞到结果就绪；有响应性要求时用 `wait_for` 或 `wait_until` 配合状态检查。

**`promise` 未设值即销毁。** 等待方会收到 `std::future_error` 与 `broken_promise`，必须处理这条错误路径；异常路径上要确保 `set_value` 或 `set_exception` 总有一条被执行。

**引用捕获的生命周期。** lambda 按引用捕获的局部变量必须活得比任务久；`std::async` 的任务可能延迟执行，悬空引用不会在启动时暴露。

**没有取消机制。** 标准 `future` 不能取消正在运行的任务；协作式取消要自行传 `std::stop_token`（C++20）或原子标志。

## 库联动

- **`<future>`**：`future`、`shared_future`、`promise`、`packaged_task` 与 `async` 都在这个头文件里。
- **`<thread>`**：`std::async` 的 `launch::async` 策略本质上启动线程；大量任务应改用线程池而不是逐个 `async`。
- **`<chrono>`**：`wait_for` 与 `wait_until` 接收时长或时间点，用来实现超时和轮询。
- **C++20 协程**：`co_await` 提供更细粒度的挂起与恢复；drogon 的证据展示了协程与 `future` 共存的桥接方式。
- **stdexec / Asio**：需要结构化并发、取消和调度器抽象时，这些库在 `future` 之上提供了更完整的模型。

## 横向对比

| 方案           | 结果获取       | 异常传播   | 取消       | 主要代价                         |
| -------------- | -------------- | ---------- | ---------- | -------------------------------- |
| `std::async`   | `future.get()` | 支持       | 不支持     | 线程创建不可控，默认策略不确定   |
| `std::thread`  | 手动共享变量   | 需自行处理 | 需自行处理 | 样板代码多，所有权和 join 易出错 |
| `std::promise` | `future.get()` | 支持       | 需自行建模 | 需要显式配对，忘记设值会 broken  |
| 回调           | 参数传入       | 需约定     | 取决于实现 | 组合与错误处理容易失控           |
| 协程（C++20）  | `co_await`     | 支持       | 支持       | 需要调度器和协程框架             |

跨标准演进：C++11 通过 N2996 引入 `future` 与 `std::async`；C++17 引入 `std::future` 的 `then` 提案没有进入标准，实际延续的是 C++20 协程；C++20 的 `std::stop_token` 补齐了协作式取消。`future` 的阻塞式接口至今仍是跨线程取结果的最小公共抽象。
