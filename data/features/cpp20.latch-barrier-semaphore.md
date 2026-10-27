---
id: cpp20.latch-barrier-semaphore
title: 闩锁、屏障与计数信号量
standard: C++20
kind: library
category: [并发, 线程同步]
difficulty: 3
status: final
tier: A
compilers: {gcc: "11", clang: "11", msvc: "19.28"}
tags: [latch, barrier, semaphore, counting_semaphore, 同步原语]
alternatives: [条件变量加计数器, std::future 与 std::promise, 自定义线程池栅栏, 操作系统信号量]
proposal: P1135R6
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用 std::latch 让每个工作线程命名后报到，发起方只调用 wait"
    evidence: [ev-nano-latch-thread-names-001]
  - id: c2
    statement: "把 latch 计数初始化为线程数，作为线程池成员复用同一份汇合点"
    evidence: [ev-nano-latch-member-002]
  - id: c3
    statement: "用固定上限的 counting_semaphore 限制同时进行的搜索数量"
    evidence: [ev-vroom-semaphore-search-001]
  - id: c4
    statement: "为线程池中的外部路由调用获取信号量许可，限制并发请求数"
    evidence: [ev-vroom-semaphore-released-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

一次性汇合、周期性同步和并发配额，过去都要用条件变量加计数器手写。计数器的初始化、递减时机、等待方的唤醒条件全部靠约定，稍有偏差就变成丢唤醒或永久阻塞。`std::shared_mutex` 只能解决读写互斥，线程之间的“等到 N 个都到达”仍然要自己搭。

C++20 的同步库提供三种原语，分别覆盖三个场景：`std::latch` 是一次性倒计数，计到零后永久打开；`std::barrier` 是可复用的会合点，每轮到达后自动复位并可选执行完成函数；`std::counting_semaphore<N>` 用许可证限制同时进入某段代码的线程数。标准提案为 P1135R6，头文件分别是 `<latch>`、`<barrier>` 和 `<semaphore>`。

## 基础用法

`std::latch` 构造时给出计数值，`count_down` 递减，`wait` 阻塞到零，`arrive_and_wait` 两者合并。

<!-- compile -->

```cpp
#include <iostream>
#include <latch>
#include <thread>

int main() {
  std::latch start(1);
  int value = 0;
  std::thread worker([&] {
    start.wait();
    value = 42;
  });

  start.count_down();
  worker.join();

  std::cout << value << '\n';
}
```

输出 `42`。`start.count_down()` 返回后 latch 计数归零，工作线程的 `wait` 随即放行；`join` 保证对 `value` 的写入在线程结束后才被读取。完整用法见 [`examples/cpp20/latch_barrier_semaphore.cpp`](../../examples/cpp20/latch_barrier_semaphore.cpp)，其中还演示了 `std::barrier` 与 `std::counting_semaphore`。

三者的差别在于生命周期：latch 只能倒数一次；barrier 每轮重置，适合反复同步的多阶段流水线；semaphore 不关心“到齐”，只控制同时持有许可的数量。

## 进阶用法

`std::barrier` 的构造函数接受参与线程数和完成函数。每轮最后一个到达的线程执行完成函数，随后所有线程一起进入下一轮。完成函数可用来在阶段之间交换缓冲区、推进迭代器或记录统计值；它不能修改参与线程的状态，因为此时其他线程尚未被放行。

`std::counting_semaphore<N>` 的模板参数是编译期最大许可数，构造参数是初始许可数。`acquire` 拿不到许可时阻塞，`try_acquire` 立即返回，`try_acquire_for` 与 `try_acquire_until` 支持超时。`release` 可以一次归还多个许可。

`std::binary_semaphore` 是 `counting_semaphore<1>` 的别名，可以当作事件或轻量互斥使用。RAII 包装需要自己写，标准没有提供 `std::lock_guard` 式的信号量卫兵。

latch 与 barrier 的等待没有超时接口，也不可取消。需要带超时的等待时，仍要退回条件变量。

## 真实项目证据

| 项目  | 位置                                                                                                                                                                                        | 用法                                                                  | 为什么典型                                     |
| ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------- | ---------------------------------------------- |
| nano  | [`nano/lib/thread_pool.hpp#L127-L137`](https://github.com/nanocurrency/nano-node/blob/6b5f63ce04c9a1ffe37b8c687fffc1bef941bf98/nano/lib/thread_pool.hpp#L127-L137)                          | 向线程池投递命名任务，任务内调用 `arrive_and_wait`，发起方调用 `wait` | 一次性汇合，latch 语义正好匹配                 |
| nano  | [`nano/lib/thread_pool.hpp#L25-L28`](https://github.com/nanocurrency/nano-node/blob/6b5f63ce04c9a1ffe37b8c687fffc1bef941bf98/nano/lib/thread_pool.hpp#L25-L28)                              | 构造函数里用线程数初始化 `thread_names_latch`                         | 计数来源与实际参与线程一一对应                 |
| VROOM | [`src/problems/vrp.h#L205-L207`](https://github.com/VROOM-Project/vroom/blob/43dd7d0b8b560431eb555bf335cf4797eb7343c4/src/problems/vrp.h#L205-L207)                                         | 用 `counting_semaphore<32>` 把并发搜索限制在硬件线程数以内            | 固定上限在编译期给出，运行期按实际线程数初始化 |
| VROOM | [`src/structures/vroom/input/input.cpp#L1264-L1274`](https://github.com/VROOM-Project/vroom/blob/43dd7d0b8b560431eb555bf335cf4797eb7343c4/src/structures/vroom/input/input.cpp#L1264-L1274) | 线程池任务先 `acquire` 再调用外部路由服务                             | 并发配额与线程数量解耦，保护外部服务           |

四条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。nano 的两条互为一对，展示计数初始化与等待点如何配合；VROOM 展示信号量在真实 I/O 边界上的用法。

## 陷阱与边界

**latch 不能复位。** 计数归零后 `count_down` 是未定义行为，`wait` 永远立即返回。需要重复汇合时用 `std::barrier`，不要试图重置 latch。

**参与线程数必须一致。** barrier 的构造函数参数与实际调用 `arrive_and_wait` 的线程数不符时，要么永久阻塞，要么提前放行。线程数在运行期变化时，需要重建 barrier 或改用其它机制。

**完成函数里不要阻塞。** barrier 的完成函数在最后一个线程上执行，其余线程都在等它。在其中获取同一 barrier 或长时间持锁会导致死锁。

**许可必须成对归还。** 信号量的 `acquire` 与 `release` 要保证路径完整，包括异常路径。RAII 卫兵需要自己实现，否则抛异常时许可永久泄漏。

**最大许可数是编译期常量。** `counting_semaphore<N>` 的 `N` 不能运行期调整，超过上限的 `release` 是未定义行为。运行期配额应当通过初始值而非模板参数控制。

**等待不可取消。** latch、barrier 和信号量的阻塞等待都不接受 `stop_token`，线程无法中途放弃。需要可取消的同步时，配合条件变量或自己封装。

## 库联动

- **线程池**：nano 在线程池构造阶段用 latch 等待所有工作线程完成命名，`std::jthread` 或 `std::thread` 数组都可以作为执行者。
- **任务调度**：VROOM 在求解器与外部路由服务两侧都用信号量限流，说明配额控制可以独立于线程创建。
- **条件变量**：需要超时、可取消或多条件等待时，条件变量仍不可替代；三者定位互补而非替代。
- **原子量与内存序**：同步原语内部已建立 happens-before，配合普通变量即可；不要再手工叠加内存序。

## 横向对比

| 原语                      | 是否可复用 | 计数语义     | 超时支持 | 适用场景                     |
| ------------------------- | ---------- | ------------ | -------- | ---------------------------- |
| `std::latch`              | 否         | 递减到零     | 无       | 一次性启动、汇合、等待初始化 |
| `std::barrier`            | 是         | 每轮自动复位 | 无       | 多阶段流水线、反复会合       |
| `std::counting_semaphore` | 是         | 许可证增减   | 有       | 并发配额、资源池限流         |
| 条件变量加计数器          | 是         | 自定义       | 有       | 需要复杂谓词或可取消等待     |

跨标准演进：C++20 通过 P1135R6 引入三种同步原语；C++20 的 `std::stop_token` 没有接入这些等待接口，C++26 的执行器模型则倾向于用发送者/接收者表达同类同步。
