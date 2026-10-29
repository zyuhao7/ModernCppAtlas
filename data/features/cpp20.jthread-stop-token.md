---
id: cpp20.jthread-stop-token
title: std::jthread 与停止令牌
standard: C++20
kind: library
category: [并发]
difficulty: 3
status: final
tier: B
compilers: {gcc: "10", clang: "20", msvc: "19.28"}
tags: [jthread, stop_token, 线程, 取消, 并发]
alternatives: [std::thread 加原子标志, std::async, 线程池, 平台线程 API]
proposal: P0660R10
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "用 std::jthread 启动后台 COM 线程，并在 lambda 里轮询 stop_token"
    evidence: [ev-gw2radial-jthread-com-thread-001]
  - id: c2
    statement: "把 std::jthread 保存在类成员中，由显式 reset 触发停止与 join"
    evidence: [ev-gw2radial-jthread-member-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

`std::thread` 析构时若仍可 join，进程会直接终止；忘记 `join` 或 `detach` 是常见的崩溃来源。线程之间的停止也一直靠手工约定：额外维护一个 `atomic<bool>`，所有阻塞点都得自己检查，调用方还要负责通知条件变量并等待退出。

`std::jthread` 把两件事合并进类型：析构函数先请求停止，再 `join`；停止请求通过标准的 `std::stop_source`/`std::stop_token` 协议传播。线程函数如果能接收第一个 `std::stop_token` 参数，`jthread` 会自动把自己的令牌传进去。取消仍是协作式的——令牌只表达“该停了”，不会强行终止线程。

## 基础用法

带 `stop_token` 参数的可调用对象会收到线程自带的令牌；析构时 `jthread` 请求停止并等待线程结束。

<!-- compile -->

```cpp
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stop_token>
#include <thread>

int main() {
  std::mutex mutex;
  std::condition_variable_any ready;

  std::jthread worker([&](std::stop_token token) {
    std::unique_lock lock(mutex);
    ready.wait(lock, token, [] { return false; });
  });

  worker.request_stop();
  ready.notify_all();

  std::cout << std::boolalpha << worker.joinable() << '\n';
}
```

输出 `true`：`request_stop()` 之后线程尚未被 join，`joinable()` 仍为真；离开作用域时 `jthread` 的析构函数完成 join。完整示例没有对应的 `examples/` 文件，因为仓库的单文件流程只编译不运行并发代码。

`condition_variable_any` 提供接收 `stop_token` 的 `wait` 重载，等待期间收到停止请求会直接返回，不需要额外的唤醒标志；普通的 `std::condition_variable` 没有这个重载。

## 进阶用法

`stop_source` 持有停止状态，`stop_token` 观察它。`jthread` 内部各有一个 `stop_source`，通过 `get_stop_source()` 和 `get_stop_token()` 暴露；也可以把外部 `stop_source` 的令牌传进线程函数，让多个线程共享同一取消信号。

`request_stop()` 返回布尔值：第一次请求返回 `true`，重复请求返回 `false`。`stop_requested()` 查询状态，`stop_possible()` 判断是否还可能收到停止请求（没有关联 `stop_source` 时为假）。

`std::stop_callback` 在构造时注册回调，停止请求发生时同步执行。回调运行在调用 `request_stop()` 的线程上，因此不能阻塞，也不应抛异常。若注册时停止已经请求，回调会在构造函数里立即执行。

阻塞点需要真正可中断。`condition_variable_any` 的 `stop_token` 重载、`std::stop_callback` 配合阻塞 I/O 或 `poll` 的超时，都属于协作式取消；裸 `read`、`recv`、`sleep_for` 不会因为令牌被请求而返回。

## 真实项目证据

| 项目      | 位置                                                                                                                                        | 用法                                                                               | 为什么典型                                     |
| --------- | ------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------- | ---------------------------------------------- |
| GW2Radial | [`src/Core.cpp#L121-L133`](https://github.com/Friendly0Fire/GW2Radial/blob/3178575f74dc68718b1608faeda5f93947f6a563/src/Core.cpp#L121-L133) | `make_unique<std::jthread>`，lambda 接收 `stop_token`，循环检查 `stop_requested()` | 后台线程的创建、取消条件与自动 join 集中在一处 |
| GW2Radial | [`include/Core.h#L93`](https://github.com/Friendly0Fire/GW2Radial/blob/3178575f74dc68718b1608faeda5f93947f6a563/include/Core.h#L93)         | `std::unique_ptr<std::jthread> comThread_;` 作为类成员                             | 用指针控制销毁时机，避免线程晚于依赖对象结束   |

两条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。项目在 `InnerShutdown()` 中显式 `comThread_.reset()`，先请求停止并 join，再清理其他成员；这比依赖成员析构顺序更明确。

## 陷阱与边界

**取消是协作式的。** `jthread` 不会杀死线程，只会设置停止状态并等待。线程若从不检查令牌、也不使用可中断的等待原语，析构函数会一直阻塞到线程自然结束。

**析构函数可能长时间阻塞。** 析构里的 `join` 没有超时。需要限时退出时，必须在外部逻辑中先请求停止并等待，确认线程结束后再销毁 `jthread`。

**停止回调在请求线程上同步执行。** 回调里做重活或再次调用可能阻塞的操作，会拖住调用 `request_stop()` 的线程；回调抛异常会直接传播到请求方。

**成员声明顺序会影响安全。** 成员按声明的逆序析构。若 `jthread` 声明在互斥量、条件变量之前，它会最后才被销毁，线程可能在依赖对象已经析构后继续运行。要么把线程成员声明在依赖对象之后，要么像 GW2Radial 一样在清理函数里显式 `reset()`。

**不接收 `stop_token` 的函数不会获得令牌。** `jthread` 只在可调用对象能以 `stop_token` 为首参调用时传入令牌，否则按无参调用；此时 `request_stop()` 仍可用，但线程函数拿不到令牌。需要取消时，应由外部通过 `jthread::get_stop_token()` 取出令牌并传入，或自行实现停止标志。

**Clang 支持较晚。** cppreference 将 Clang 17 标为部分支持、Clang 20 标为完整支持；以 Clang 为目标的项目要先用最小示例验证 `stop_token` 相关库设施。

**不能强制终止。** 需要硬取消时只能依赖平台 API 或进程隔离；`jthread` 的模型刻意不提供强制终止。

## 横向对比

| 方案                           | 自动 join             | 标准取消协议               | 主要代价                           |
| ------------------------------ | --------------------- | -------------------------- | ---------------------------------- |
| `std::thread` + `atomic<bool>` | 否                    | 手写                       | 忘记 join 会终止进程，停止逻辑分散 |
| `std::jthread`                 | 是                    | `stop_token`/`stop_source` | 析构可能阻塞，取消仍需协作         |
| `std::async`                   | future 析构时可能阻塞 | 无                         | 取消能力弱，线程归属不明确         |
| 线程池                         | 由池管理              | 各库自定义                 | 需要额外依赖，任务粒度受池设计限制 |

跨标准演进：C++20 通过 P0660R10 引入 `std::jthread`、`std::stop_token`、`std::stop_source` 与 `std::stop_callback`；它们把“join + 协作取消”固化成标准接口，但没有改变协作式取消的基本约束。
