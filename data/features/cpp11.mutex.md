---
id: cpp11.mutex
title: 互斥量与作用域锁（std::mutex）
standard: C++11
kind: library
category: [并发, 线程同步]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "19.0"}
tags: [互斥量, lock_guard, 临界区, 数据竞争, RAII]
alternatives: [std::atomic, std::condition_variable, C++17 scoped_lock, 无锁数据结构]
proposal: N2320
last_verified: "2026-11-10"
claims:
  - id: c1
    statement: "日志 sink 写入用 lock_guard 保护共享状态"
    evidence: [ev-spdlog-mutex-base-sink-001]
  - id: c2
    statement: "采集监控指标时持锁复制一致快照"
    evidence: [ev-drogon-mutex-gauge-snapshot-002]
  - id: c3
    statement: "代表权重缓存与计数器在同一临界区内同步更新"
    evidence: [ev-nano-mutex-rep-weights-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

多个线程同时读写同一份数据而没有同步，就是数据竞争：`++counter` 是读、加、写三步，两个线程可能都读到旧值，最终少加一次。这种错误不会稳定复现，却会随负载升高而出现。

C++11 在 `<mutex>` 中提供 `std::mutex`：`lock()` 获取所有权，`unlock()` 释放，二者之间形成临界区。由于手动配对容易在异常或提前返回时漏解锁，标准库同时提供 RAII 包装 `std::lock_guard`，构造时加锁、析构时解锁。标准提案为 N2320。

## 基础用法

用 `std::lock_guard<std::mutex>` 包住临界区，锁的生命周期就是作用域。

<!-- compile -->

```cpp
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

int main() {
  std::mutex mutex;
  int counter = 0;

  auto work = [&] {
    for (int i = 0; i < 1000; ++i) {
      std::lock_guard<std::mutex> lock(mutex);
      ++counter;
    }
  };

  std::vector<std::thread> workers;
  for (int i = 0; i < 4; ++i) {
    workers.emplace_back(work);
  }
  for (auto& worker : workers) {
    worker.join();
  }

  std::cout << counter << '\n';
}
```

输出恒为 `4000`。四个线程各自增一千次，每次自增都处于同一把互斥量保护的临界区内，没有丢失更新。完整版本见 [`examples/cpp11/mutex.cpp`](../../examples/cpp11/mutex.cpp)，按四个场景展开，输出如下：

```text
[1] counter = 4000
[2] left = 10000, right = 10000, mismatches = 0
[3] lock released after exception = true
[4] recursive depth = 3
```

场景 2 让写线程在同一把锁内更新 `left` 与 `right`，读线程也在锁内复制整个结构体，所以看不到更新到一半的快照；场景 3 在临界区内抛出异常，`lock_guard` 析构时仍然解锁，随后 `try_lock()` 能再次成功；场景 4 用 `std::recursive_mutex` 允许同一线程递归进入，深度计数器每层加一。

## 进阶用法

**`std::mutex` 默认不可重入。** 同一线程对已持有的 `std::mutex` 再次 `lock()` 是未定义行为，通常会死锁；需要重入时使用 `std::recursive_mutex`。

**`lock_guard` 不可移动。** 它只负责最简单的构造加锁、析构解锁；需要中途解锁、延迟加锁或转移所有权时改用 `std::unique_lock`，后者可以移动。

**用 `std::lock` 同时锁多个互斥量。** `std::lock(m1, m2)` 内部按死锁避免算法加锁，再用 `std::lock_guard` 配合 `std::adopt_lock` 接管；不要手写固定顺序以外的加锁逻辑。

**锁保护的是约定，不是数据。** 互斥量不会自动关联任何变量；所有访问同一份数据的路径必须使用同一把锁，漏掉一处就等于没有同步。

**临界区尽量小。** 持锁期间做文件 I/O、网络请求或调用未知回调，会把等待时间放大到所有线程；只把真正共享的读写放进临界区。

**锁与条件变量分工不同。** 等待某个条件成立时不能靠轮询加锁，应使用 `std::condition_variable` 配合 `std::unique_lock`，在条件满足前释放互斥量。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                         | 用法                               | 为什么典型                                 |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------- | ------------------------------------------ |
| spdlog | [`include/spdlog/sinks/base_sink-inl.h#L25-L29`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/sinks/base_sink-inl.h#L25-L29)                | 日志 sink 写入用 `lock_guard` 保护 | 锁对象与临界区同作用域，异常路径也会解锁   |
| drogon | [`lib/inc/drogon/utils/monitoring/Gauge.h#L40-L48`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/utils/monitoring/Gauge.h#L40-L48) | 采集指标时持锁复制快照             | 读路径也加锁，避免看到更新到一半的字段     |
| nano   | [`nano/secure/rep_weights.cpp#L17-L23`](https://github.com/nanocurrency/nano-node/blob/6b5f63ce04c9a1ffe37b8c687fffc1bef941bf98/nano/secure/rep_weights.cpp#L17-L23)                         | 缓存与计数器在同一临界区内更新     | 多个相关字段必须一起可见，读侧才能保持一致 |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖写入、读取与多字段更新三种临界区形态。

## 陷阱与边界

**忘记加锁的读同样危险。** 数据竞争不只发生在写写之间；一个线程写、另一个线程无锁读同样是未定义行为。

**持锁调用外部代码会扩大死锁面。** 回调里可能再次获取同一把锁或获取另一把锁，形成锁顺序反转；临界区内只做必要的内存操作。

**异常安全依赖 RAII。** 手写 `lock()`/`unlock()` 时，中间抛异常会永久持锁；`lock_guard` 与 `unique_lock` 在析构时解锁，是默认选择。

**不要按值传递互斥量。** `std::mutex` 不可拷贝也不可移动；把它放进按值捕获的 lambda 或按值参数都会编译失败，需要传引用或使用 `std::shared_ptr`。

**`lock_guard` 不提供 `try_lock`。** 需要非阻塞尝试时使用 `std::unique_lock` 的 `try_lock()` 或直接调用 `mutex.try_lock()`。

**MSVC 支持较晚。** VS2015（19.0）才提供完整可用的 `<mutex>`；更老的工具链需要平台 API 或第三方库封装。

## 库联动

- **`<mutex>`**：`std::mutex`、`std::recursive_mutex`、`std::timed_mutex`、`lock_guard`、`unique_lock` 与 `std::lock` 同属一套设施。
- **`std::condition_variable`**：等待条件时使用 `std::unique_lock`，在阻塞前释放互斥量。
- **`std::lock_guard` / `std::unique_lock`**：作用域锁把加解锁与异常路径绑定，避免手写配对。
- **`std::atomic`**：单个标量的读改写优先用原子类型；互斥量适合保护多个字段或非原子操作。
- **`std::once_flag`**：一次性初始化与互斥量常配合出现，但 `call_once` 独立于具体互斥量。
- **spdlog / drogon / nano-node**：分别展示写路径、读路径与多字段更新的加锁方式。

## 横向对比

| 方案                         | 保护范围   | 阻塞行为       | 主要限制                                 |
| ---------------------------- | ---------- | -------------- | ---------------------------------------- |
| `std::mutex` + `lock_guard`  | 任意临界区 | 阻塞等待       | 不可重入，需统一约定锁与数据的对应关系   |
| `std::atomic`                | 单个标量   | 通常无锁       | 复合操作仍需 compare-exchange 循环       |
| `std::shared_mutex`（C++17） | 读多写少   | 读共享、写独占 | C++11 不可用，读锁也非零成本             |
| `std::scoped_lock`（C++17）  | 多个互斥量 | 阻塞等待       | C++11 只能用 `std::lock` 加 `adopt_lock` |

跨标准演进：C++11 通过 N2320 引入 `<mutex>`、`lock_guard` 与 `unique_lock`；C++14 增加 `std::shared_lock` 的配套类型；C++17 引入 `std::scoped_lock` 与 `std::shared_mutex`，把多锁死锁避免和读写锁纳入标准；C++20 的 `std::jthread` 改善线程生命周期，但互斥量的基本语义未变。
