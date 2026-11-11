---
id: cpp11.condition-variable
title: std::condition_variable
standard: C++11
kind: library
category: [并发, 线程同步]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "16.0"}
tags: [condition_variable, 条件变量, 等待, 通知, 生产者消费者]
alternatives: [忙等, 信号量, std::atomic wait, 事件队列]
proposal: N2497
last_verified: "2026-11-03"
claims:
  - id: c1
    statement: "用 condition_variable::wait_for 配合谓词实现可中断的周期任务"
    evidence: [ev-spdlog-condvar-periodic-worker-001]
  - id: c2
    statement: "用 condition_variable::wait 等待队列腾出空间"
    evidence: [ev-spdlog-condvar-queue-not-full-002]
  - id: c3
    statement: "线程池工作线程在无任务时用 condition_variable::wait 休眠"
    evidence: [ev-stdexec-condvar-worker-sleep-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

线程要等待另一个线程把数据准备好，最直接的办法是循环检查一个标志。忙等会占满 CPU，还会因为编译器优化或缓存可见性写出永远退不出的循环。互斥量只能保护数据，不能让线程“睡着等条件成立”。

`std::condition_variable` 把“释放锁并挂起”和“被通知后重新加锁”合成一个原子式的等待：`wait` 在进入等待前解锁互斥量，被唤醒后重新加锁再返回，调用方因此可以在持锁状态下检查条件。标准提案为 N2497，头文件是 `<condition_variable>`。

## 基础用法

`wait` 必须配合 `std::unique_lock<std::mutex>`；等待期间锁被释放，其他线程可以修改共享状态并调用 `notify_one` 或 `notify_all`。始终使用带谓词的重载，它能处理虚假唤醒并减少手写循环。

<!-- compile -->

```cpp
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

int main() {
  std::mutex mutex;
  std::condition_variable cv;
  std::queue<int> queue;
  bool done = false;

  std::thread producer([&] {
    for (int value = 1; value <= 3; ++value) {
      {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(value);
      }
      cv.notify_one();
    }
    {
      std::lock_guard<std::mutex> lock(mutex);
      done = true;
    }
    cv.notify_one();
  });

  std::thread consumer([&] {
    int count = 0;
    int total = 0;
    while (true) {
      std::unique_lock<std::mutex> lock(mutex);
      cv.wait(lock, [&] { return !queue.empty() || done; });
      while (!queue.empty()) {
        total += queue.front();
        queue.pop();
        ++count;
      }
      if (done) {
        break;
      }
    }
    std::cout << count << ' ' << total << '\n';
  });

  producer.join();
  consumer.join();
}
```

输出 `3 6`。生产者在锁内把 1、2、3 放入队列并通知；消费者在谓词成立前持续等待，醒来后一次排空队列，看到 `done` 后退出。完整版本见 [`examples/cpp11/condition_variable.cpp`](../../examples/cpp11/condition_variable.cpp)，三个场景的输出为：

```text
[1] count = 3, total = 6
[2] timeout with false predicate = true
[3] ready predicate = true
```

场景 1 是标准的生产者/消费者：`lock_guard` 只在写队列时持有锁，`notify_one` 在解锁之后发出；消费者用 `unique_lock` 配合带谓词的 `wait`，被唤醒后一次排空队列。场景 2 用 `wait_for` 加 10ms 超时和恒假谓词，稳定返回 `false`，说明超时路径的返回值。场景 3 在等待前先把条件置真，`wait_for` 立即返回 `true`，不会真的等满超时。

谓词由调用方提供，`wait(lock, pred)` 等价于“循环检查 `pred()`，为假就等待”。条件变量本身不保存状态，状态必须放在共享变量里，并由同一把互斥量保护。

## 进阶用法

通知可以在持锁或不持锁时发出。持锁通知会让被唤醒线程立刻又阻塞在锁上，通常先解锁再 `notify_one`；但不持锁通知要求共享状态已经被正确发布。两种写法都正确，关键是通知前状态已经更新。

`notify_one` 只唤醒一个等待者，适合生产者/消费者队列；`notify_all` 唤醒全部等待者，适合状态变化会影响所有线程的场景，例如关闭标志。被唤醒的线程仍需重新竞争锁并重新检查谓词。

`wait_for` 和 `wait_until` 提供超时版本，返回 `std::cv_status` 或谓词结果。周期任务常用 `wait_for(lock, interval, pred)`：超时说明该执行下一轮，谓词成立说明收到停止信号。

条件变量不能复制，也不能在等待期间销毁。等待线程持有的 `unique_lock` 会在 `wait` 内部解锁再重新加锁，因此等待期间不能依赖锁的保护访问共享状态。

C++20 的 `std::atomic::wait` 与 `std::counting_semaphore` 覆盖了一部分场景：只需要等待一个整数或计数的线程，可以不再单独配互斥量与条件变量。

## 真实项目证据

| 项目    | 位置                                                                                                                                                                                  | 用法                                       | 为什么典型                                             |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------ | ------------------------------------------------------ |
| spdlog  | [`include/spdlog/details/periodic_worker.h#L31-L39`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/periodic_worker.h#L31-L39) | 用 `wait_for` 加谓词实现可中断的周期任务   | 超时执行任务与停止信号合并到一次等待，析构时能立即退出 |
| spdlog  | [`include/spdlog/details/mpmc_blocking_q.h#L31-L38`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/mpmc_blocking_q.h#L31-L38) | 用 `wait` 等待有界队列腾出空间             | 谓词检查队列未满，避免虚假唤醒后越界写入               |
| stdexec | [`include/exec/static_thread_pool.hpp#L1145-L1159`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/static_thread_pool.hpp#L1145-L1159)  | 工作线程无任务时在条件变量上休眠，避免空转 | 原子状态转换与条件变量等待配合，控制线程的睡眠与唤醒   |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖周期任务、有界队列与线程池休眠。

## 陷阱与边界

**等待必须带谓词。** 条件变量可能虚假唤醒，也可能在通知与等待之间错过通知；`wait(lock, pred)` 会重新检查条件，裸 `wait(lock)` 需要自己写循环。

**丢失唤醒。** 如果通知发生在等待线程检查条件之后、真正进入等待之前，通知会丢失；把状态更新与谓词检查放在同一把锁下可以避免这个窗口。

**`wait` 需要 `unique_lock`。** `condition_variable` 要求等待期间能解锁并重新加锁，`lock_guard` 做不到；`condition_variable_any` 接受任意可锁类型，但更重。

**通知前必须更新状态。** 只调用 `notify_one` 而共享变量没变，被唤醒线程会重新检查谓词并继续等待；通知本身不是消息。

**析构时不能有等待者。** 销毁仍有线程在 `wait` 的条件变量是未定义行为；关闭流程要先设置停止标志、`notify_all`，再 join 所有等待线程。

**`notify_one` 可能唤醒错误线程。** 多个等待者对应不同条件时，单个通知可能被无关线程消费；按条件分组使用不同条件变量，或改用 `notify_all`。

**与互斥量的锁顺序要一致。** 条件变量不解决死锁；等待线程和通知线程都必须以同一顺序获取相关锁。

## 库联动

- **`std::mutex` / `std::unique_lock`**：条件变量的标准搭档；`wait` 在内部解锁并在返回前重新加锁。
- **`std::queue` / 环形缓冲**：生产者/消费者模型的共享状态，通常与有界队列和两个条件变量配合。
- **`std::thread`**：等待线程与通知线程的创建和汇合；关闭顺序通常是“置停止位 → notify_all → join”。
- **`std::atomic`**：简单标志可以用原子量加 `wait`/`notify` 替代条件变量，减少一把锁。
- **`std::counting_semaphore`**：C++20 的信号量适合纯计数场景，不必手写谓词。

## 横向对比

| 方案                      | 需要互斥量 | 虚假唤醒处理     | 超时支持 | 主要代价                         |
| ------------------------- | ---------- | ---------------- | -------- | -------------------------------- |
| `std::condition_variable` | 是         | 谓词重载自动处理 | 是       | 必须正确配对状态、锁与通知       |
| 忙等                      | 否         | 不适用           | 是       | 占满 CPU，容易写出不可见循环     |
| `std::atomic::wait`       | 否         | 平台负责         | 否       | C++20 起可用，只适合单值条件     |
| `std::counting_semaphore` | 否         | 不适用           | 是       | C++20 起可用，语义偏计数而非谓词 |

跨标准演进：C++11 通过 N2497 引入 `condition_variable` 与 `condition_variable_any`；C++20 增加 `atomic::wait`/`notify` 与信号量，为简单等待场景提供更轻的替代；条件变量的核心“锁 + 谓词 + 通知”模型没有变化。
