---
id: cpp11.atomic
title: std::atomic
standard: C++11
kind: library
category: [并发, 原子操作]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.1", msvc: "17.0"}
tags: [atomic, 原子操作, 内存序, 无锁, 数据竞争]
alternatives: [std::mutex, std::atomic_ref, 平台原子内建, 无锁队列]
proposal: N2427
last_verified: "2026-11-03"
claims:
  - id: c1
    statement: "用 std::atomic<bool> 保存跨线程读取的启用标志"
    evidence: [ev-spdlog-atomic-backtracer-flag-001]
  - id: c2
    statement: "用 std::atomic<size_t> 维护定时轮的 tick 计数"
    evidence: [ev-drogon-atomic-tick-counter-002]
  - id: c3
    statement: "用 std::atomic_bool 在并行符号解析中记录需要重试的状态"
    evidence: [ev-mold-atomic-flag-resolve-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

多个线程读写同一个普通变量时，语言层面就是数据竞争，行为未定义。用互斥量可以消除竞争，但为一个计数器或布尔标志加锁，既增加阻塞，也要求所有访问路径都记得加同一把锁。

`std::atomic<T>` 把原子操作做进类型：`load`、`store`、`exchange`、`compare_exchange` 和整型的 `fetch_add` 等操作不可分割，其他线程要么看到操作前的结果，要么看到操作后的结果。标准提案为 N2427，头文件是 `<atomic>`。

## 基础用法

原子对象的操作通过成员函数完成，读写默认使用 `memory_order_seq_cst`。`fetch_add` 返回旧值，`compare_exchange_strong` 在期望值匹配时写入新值，`exchange` 返回旧值并写入新值。

<!-- compile -->

```cpp
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  std::atomic<int> counter{0};
  std::atomic<bool> ready{false};
  std::vector<std::thread> workers;

  for (int i = 0; i < 4; ++i) {
    workers.emplace_back([&counter, &ready] {
      counter.fetch_add(1, std::memory_order_relaxed);
      ready.store(true, std::memory_order_release);
    });
  }
  for (auto& worker : workers) {
    worker.join();
  }

  std::cout << counter.load() << ' '
            << ready.load(std::memory_order_acquire) << '\n';

  int expected = counter.load();
  const bool swapped = counter.compare_exchange_strong(expected, 100);
  std::cout << swapped << ' ' << counter.load() << '\n';

  const int previous = counter.exchange(7);
  std::cout << previous << ' ' << counter.load() << '\n';
}
```

输出 `4 1`、`1 100` 与 `100 7`。四个线程各做一次 `fetch_add`，主线程 join 后读到 4；`compare_exchange_strong` 把 4 换成 100 并返回成功，随后 `exchange(7)` 返回旧值 100。完整版本见 [`examples/cpp11/atomic.cpp`](../../examples/cpp11/atomic.cpp)，四个场景的输出为：

```text
[1] counter = 4000
[2] payload after acquire = 99
[3] first = true, second = false, stale expected = 100, slot = 100
[4] exchange returned 4000, counter now = 1
```

场景 1 让四个线程各做 1000 次 `fetch_add`，`relaxed` 足够，最终计数确定是 4000。场景 2 演示发布/获取：写线程先写普通变量 `payload`，再用 `release` 置标志；读线程用 `acquire` 自旋等待，看到标志时 `payload` 必然可见，读到 99。场景 3 是 `compare_exchange` 的期望值陷阱：第一次成功把 4 换成 100；第二次传入过期的期望值 0 失败，`stale` 被改写成当前值 100，`slot` 保持 100。场景 4 的 `exchange` 原子地返回旧值 4000 并写入 1。

`T` 必须是可平凡复制的类型；浮点数的 `fetch_add` 到 C++20 才进入标准，C++11 下浮点原子只保证 `load`、`store`、`exchange` 和比较交换。需要 `atomic_ref` 语义时，C++20 提供对已有对象的原子引用。

## 进阶用法

内存序决定原子操作与周围普通访问的可见性。`relaxed` 只保证操作本身原子，不建立同步；`acquire`/`release` 成对使用，让发布线程在 release 之前的写入对获取线程可见；`seq_cst` 额外提供全局单一顺序，代价最高。

计数器、统计量通常只需要 `relaxed` 的 `fetch_add`；用原子标志发布一块已经初始化好的数据时，写端用 `release` 存、读端用 `acquire` 读。`memory_order_consume` 在实践中难以正确使用，通常直接用 `acquire` 替代。

`compare_exchange_weak` 允许伪失败，适合放在循环里；`compare_exchange_strong` 不会伪失败，但某些平台上更贵。两者失败时都会把 `expected` 更新为当前值，循环重试前要重新读取。

`std::atomic_flag` 是保证无锁的最小类型，适合实现自旋锁；普通 `std::atomic<T>` 用 `is_lock_free()` 查询，`is_always_lock_free` 在 C++17 起可编译期判断。C++20 增加 `wait`/`notify_one`/`notify_all`，可以在原子值上等待而不必单独建条件变量。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                        | 用法                                          | 为什么典型                                   |
| ------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------- | -------------------------------------------- |
| spdlog | [`include/spdlog/details/backtracer.h#L18-L20`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/backtracer.h#L18-L20) | 用 `std::atomic<bool>` 保存调试回溯的启用标志 | 单个布尔标志用原子量即可，不必为它再加互斥量 |
| drogon | [`lib/inc/drogon/CacheMap.h#L452-L457`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/CacheMap.h#L452-L457)        | 用 `std::atomic<size_t>` 维护定时轮 tick 计数 | 高频递增的共享标量与容器锁分离，减少争用     |
| mold   | [`src/passes.cc#L318-L322`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/passes.cc#L318-L322)                                           | 用 `std::atomic_bool` 记录并行解析的重试状态  | 并行循环中共享一个轻量信号，避免整段加锁     |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖标志、计数器和并行算法信号三种形态。

## 陷阱与边界

**原子性不等于同步。** `relaxed` 操作只保证自身不可分割，不会让其他内存写入自动可见；需要发布/获取关系时必须显式选择 `release`/`acquire`。

**默认内存序最贵。** 不传内存序等价于 `seq_cst`，在热点路径上可能明显拖慢；确认不需要全局顺序后再放松。

**`compare_exchange` 会改期望值。** 失败时 `expected` 被写成当前值；如果循环里继续拿旧期望值重试，逻辑会出错。

**原子类型不保证无锁。** 较大的 `T` 可能退化为内部锁；实时性或信号安全场景要检查 `is_lock_free()`。

**不要混用原子与非原子访问。** 同一对象只要有一个线程用普通读写，竞争仍然存在；所有访问都必须走原子操作。

**原子操作不能替代复合操作。** 两个原子操作之间仍可能被其他线程插入；需要“读—改—写”整体原子时用 `compare_exchange` 循环或互斥量。

**浮点与自定义类型支持有限。** C++11 的浮点原子没有 `fetch_add`；自定义类型必须可平凡复制，且只支持 `load`/`store`/`exchange`/比较交换。

## 库联动

- **`std::mutex`**：保护复合不变量；原子量只适合单个标量或标志，二者常在同一数据结构中分工。
- **`std::memory_order`**：与原子操作配套使用；发布/获取模式是无锁队列和引用计数的基础。
- **`std::shared_ptr`**：控制块内部使用原子引用计数，但 `shared_ptr` 对象本身的读写仍需同步。
- **`std::atomic_ref`**：C++20 对已有对象提供原子访问，适合固定布局的共享内存。
- **无锁数据结构**：Michael–Scott 队列、 hazard pointer 等都以原子比较交换为原语。

## 横向对比

| 方案              | 单标量开销 | 复合操作   | 阻塞 | 主要代价                           |
| ----------------- | ---------- | ---------- | ---- | ---------------------------------- |
| `std::atomic`     | 低         | 需自己组合 | 否   | 内存序容易用错                     |
| `std::mutex`      | 较高       | 原生支持   | 是   | 争用下上下文切换，可能优先级反转   |
| 平台原子内建      | 最低       | 需自己组合 | 否   | 不可移植，内存序与类型处理手写     |
| `std::atomic_ref` | 低         | 需自己组合 | 否   | C++20 起可用，生命周期由调用方保证 |

跨标准演进：C++11 通过 N2427 引入 `std::atomic` 与六种内存序；C++17 增加 `is_always_lock_free` 和 `atomic<T>::is_always_lock_free`；C++20 增加 `atomic_ref`、浮点 `fetch_add` 与 `wait`/`notify`。
