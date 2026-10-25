---
id: cpp20.coroutines
title: 协程
standard: C++20
kind: language
category: [异步, 并发, 控制流]
difficulty: 5
status: final
tier: A
compilers: {gcc: "10", clang: "17", msvc: "19.28"}
tags: [co_await, co_yield, co_return, promise_type, 无栈协程]
alternatives: [回调, std::future, 手写状态机, 发送者/接收者]
proposal: P0912R5
last_verified: "2026-10-25"
claims:
  - id: c1
    statement: "调度器按处理器返回类型分支，在 lambda 协程内 co_await 异步结果"
    evidence: [ev-drogon-coroutine-binder-001]
  - id: c2
    statement: "在返回 Task<> 的 lambda 协程中用 try/catch 包裹 co_await，异常路径显式 co_return"
    evidence: [ev-drogon-coroutine-filter-002]
  - id: c3
    statement: "把回调式 WebSocket 事件适配成 Task<>，在 lambda 内 co_await 协程处理器"
    evidence: [ev-drogon-coroutine-websocket-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

异步代码用回调表达时，一个线性流程会被拆成多个函数：中间状态放进对象成员，错误处理分散在每一层，循环和条件分支要么重复注册回调，要么写成显式状态机。`std::future` 能等待结果，但阻塞线程，也无法表达“暂停后由别处恢复”的协作式流程。

协程让普通函数在任意点暂停和恢复。函数体仍按顺序书写，编译器把局部变量、暂停位置和恢复逻辑打包进一个**协程帧**。`co_await` 等待一个异步结果，`co_yield` 产出一个值，`co_return` 结束函数。它是**无栈协程**：暂停时局部状态在帧里，而不是保留调用栈。

协程本身不提供线程、调度器或事件循环；它只提供“可暂停的函数”这一语言机制。谁来恢复、在哪个线程恢复，由 awaiter、promise 类型和外部执行器决定。

## 基础用法

下面是一个同步拉取的生成器：

```cpp
Generator<int> range(int first, int last) {
  for (int value = first; value < last; ++value) {
    co_yield value;
  }
}

auto numbers = range(1, 5);
while (numbers.next()) {
  std::cout << numbers.value() << ' ';
}
```

`co_yield` 暂停函数并把值交给调用方；`next()` 恢复执行，直到下一个 `co_yield` 或函数结束。完整示例还实现了一个可 `co_await` 的 `Task<T>`，见 [`examples/cpp20/coroutines.cpp`](../../examples/cpp20/coroutines.cpp)：

```text
range: 1 2 3 4
task: 24
caught: boom
```

示例中 `compute` 连续 `co_await` 两个 `fetch_value`，`fail` 抛出异常并由 `Task::run` 重新抛出。三个输出分别覆盖 `co_yield`、`co_await` 链和异常传播。

## 进阶用法

**协程由返回类型决定。** 编译器在返回类型里查找嵌套的 `promise_type`。函数体包含 `co_await`、`co_yield` 或 `co_return` 时自动成为协程；`main`、构造函数和析构函数不能是协程。

**promise 协议。** 编译器依次调用 `get_return_object()` 生成返回值，用 `initial_suspend()` 决定是否立即执行，函数体结束时调用 `final_suspend()`。`co_return expr` 调用 `return_value(expr)`，`co_return;` 调用 `return_void()`；两者不能同时存在。`co_yield expr` 等价于 `co_await promise.yield_value(expr)`。

**awaiter 三件事。** `co_await expr` 通过 `operator co_await` 或直接调用成员，得到 awaiter，然后依次询问：`await_ready()` 是否可以跳过暂停；`await_suspend(handle)` 如何挂起；`await_resume()` 返回什么结果。`await_suspend` 可以返回 `void`（挂起）、`bool`（`false` 表示不挂起）或另一个 `coroutine_handle`（对称转移，直接恢复该协程而不增长调用栈）。

**惰性与急切。** `initial_suspend()` 返回 `suspend_always` 时，协程创建后停在起点，由调用方决定何时 `resume()`；返回 `suspend_never` 时立即执行到第一个暂停点。库作者必须明确选择，调用方不能假设。

**异常必须显式处理。** 函数体抛出的异常进入 promise 的 `unhandled_exception()`。通常在这里保存 `std::exception_ptr`，等 `await_resume()` 或任务驱动方取结果时重新抛出。若异常从 `unhandled_exception()` 逃出，程序终止。

**帧分配不是零成本。** 协程帧通常从堆分配；编译器可以做分配消除（HALO），但标准不保证。高频短协程应测量分配开销，或复用自定义分配器。

**恢复线程由调用方决定。** `handle.resume()` 在哪个线程执行，协程就在哪个线程继续。协程不会自动切换线程，也不保证互斥；跨线程恢复需要执行器或调度器负责同步。

**取消不是内建的。** `std::stop_token` 不会自动传播到协程。需要取消时，把 token 放进 promise 或参数，并在每个暂停点检查；awaiter 还应支持被销毁或超时。

## 真实项目证据

| 项目                   | 位置                                                                                                                                                                                       | 用法                                                  | 为什么典型                                                  |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------- | ----------------------------------------------------------- |
| drogonframework/drogon | [`lib/inc/drogon/HttpBinder.h#L408-L417`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/HttpBinder.h#L408-L417)                   | 按返回类型分支，在 `Task<>` 分支内 `co_await` 处理器  | 协程返回类型参与框架分派；`co_await` 使 lambda 自身成为协程 |
| drogonframework/drogon | [`lib/inc/drogon/HttpFilter.h#L107-L121`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/HttpFilter.h#L107-L121)                   | `try`/`catch` 包裹 `co_await`，异常路径 `co_return`   | 展示异常如何留在协程体内处理，而不是穿透 promise            |
| drogonframework/drogon | [`lib/inc/drogon/WebSocketController.h#L186-L193`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/WebSocketController.h#L186-L193) | 回调事件用 `async_run` 启动 `Task<>`，内部 `co_await` | 旧回调接口与协程业务代码之间的适配层                        |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。Drogon 的 `Task<>` 是库自定义的惰性任务类型，不是标准库类型；价值在于展示生产框架如何把协程接入既有分派与错误处理路径。

## 陷阱与边界

- **跨暂停点持有引用最容易出错。** 协程帧保存的是引用本身，不是被引用对象的生命周期。参数按引用传入、局部变量取地址、捕获 `this` 后让协程活过对象析构，都会产生悬垂引用。
- **临时对象的生命周期只到完整表达式结束。** `co_await make_awaiter()` 的临时对象在恢复后可能已经销毁；awaiter 不能在 `await_resume` 之外继续引用它。
- **协程帧必须恰好销毁一次。** 在协程仍可能被恢复时 `destroy()` 是未定义行为；`final_suspend` 返回 `suspend_always` 后由所有者销毁，返回 `suspend_never` 则自动销毁，二者不能混用。
- **惰性任务不驱动就不执行。** 创建一个 `Task` 后直接丢弃，函数体从未运行，帧随即销毁；调用方必须 `resume()` 或通过 `co_await` 接入驱动链。
- **异常不会自动到达调用方。** 忘记在 `unhandled_exception()` 保存异常，或在驱动方不检查，异常会被静默吞掉；让异常逃出 `unhandled_exception()` 则直接 `std::terminate`。
- **协程不是线程。** `co_await` 不保证让出 CPU 给其他线程，也不保证并行；它只挂起当前协程。CPU 密集任务仍需线程池或并行算法。
- **不要在锁内暂停。** 持有 `std::mutex` 时 `co_await` 可能长期占用锁；协程恢复线程也未必是加锁线程。把等待移到锁外，或改用异步感知的同步原语。
- **`await_suspend` 返回协程句柄时不要递归恢复自己。** 对称转移要求返回的句柄尚未完成；恢复一个已完成的句柄是未定义行为。
- **分配消除不可依赖。** 不要把“协程一定在栈上”当作性能前提；对热路径做基准测试。
- **调试信息与栈回溯不完整。** 暂停中的协程不在普通调用栈上，调试器需要理解协程帧；日志应记录协程标识与恢复点。

## 库联动

- **Drogon**：用自定义 `Task<>`、`async_run` 和 `co_await` 把 HTTP、过滤器、WebSocket 与 ORM 串成协程链；ORM 提供 `execSqlCoro`、`newTransactionCoro` 等协程接口。
- **C++23 `std::generator`**：标准库的同步生成器，适合把惰性序列接入 ranges；它只解决 `co_yield` 场景，不替代通用任务类型。
- **C++26 `std::execution`（P2300）**：sender/receiver 提供调度、组合与取消模型；协程可以作为其中一个 awaitable 适配层，但标准库不强制二者绑定。
- **Asio**：`asio::awaitable<T>`、`co_spawn` 与执行器组合，是网络代码中常见的协程运行时；恢复线程由关联执行器决定。
- **fmt / spdlog**：协程内可以正常格式化与记录日志；注意日志锁不要在暂停点期间持有，异步日志后端更适合高并发协程。
- **`std::stop_token`**：标准取消机制本身不是协程感知的，需要库在 promise 或 awaiter 中桥接。

## 横向对比

| 方案                         | 线性代码       | 暂停/恢复               | 异常传播               | 取消与组合            | 主要代价               |
| ---------------------------- | -------------- | ----------------------- | ---------------------- | --------------------- | ---------------------- |
| 回调                         | 否，流程被拆散 | 由回调注册隐式完成      | 需每层显式处理         | 手工维护状态          | 状态分散、易漏错误路径 |
| `std::future` / `std::async` | 部分           | 阻塞等待，不能协作暂停  | `get()` 重新抛出       | 组合能力弱            | 占用线程，难以取消     |
| 协程                         | 是             | `co_await` / `co_yield` | promise 显式存储并重抛 | 依赖 awaiter 与执行器 | 帧分配、生命周期复杂   |
| 发送者/接收者（P2300）       | 组合式         | 由调度器恢复            | 错误通道显式传递       | 内建组合与取消        | 学习曲线与库支持       |

跨标准演进：协程进入 C++20（P0912R5）；C++23 增加 `std::generator`（P2502R2）；C++26 的 `std::execution`（P2300）补齐调度与组合模型。C++17 及以前只能使用回调、future 或第三方协程库。cppreference 将 GCC 10 列为完整支持；Clang 17 与 MSVC 19.28 仍带 partial 标记，实际可用性取决于标准库与具体子特性，不能把版本号当作完整语义保证。
