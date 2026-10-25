---
id: cpp17.shared-mutex
title: std::shared_mutex
standard: C++17
kind: library
category: [并发, 读写锁]
difficulty: 3
status: final
tier: B
compilers: {gcc: "6", clang: "3.7", msvc: "19.0"}
tags: [读写锁, 共享锁, 缓存, 并发]
alternatives: [std::mutex, std::shared_timed_mutex, std::atomic, 线程本地缓存]
proposal: N4508
last_verified: "2026-10-25"
claims:
  - id: c1
    statement: "读路径用 shared_lock 快速命中，未命中再升级为 unique_lock 创建并二次检查"
    evidence: [ev-mold-shared-mutex-output-section-001]
  - id: c2
    statement: "并行任务在 shared_lock 下复制共享映射，再在线程本地缓存上工作"
    evidence: [ev-mold-shared-mutex-thread-cache-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

有些共享数据读多写少：符号表、路由表、配置缓存。用 `std::mutex` 保护时，多个只读查询也被迫串行，吞吐量随读者数量下降。用普通读写锁又常引入平台特定 API 或额外依赖。

`std::shared_mutex` 提供两种所有权：多个线程可以同时持有共享锁，但共享锁与独占锁互斥。读路径用 `std::shared_lock`，写路径用 `std::unique_lock`（或 `std::scoped_lock`），从而让并发读真正并行。

## 基础用法

```cpp
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

class ConfigCache {
public:
  std::optional<std::string> get(std::string_view key) const {
    std::shared_lock lock(mu_);
    if (auto it = values_.find(key); it != values_.end())
      return it->second;
    return std::nullopt;
  }

  void set(std::string key, std::string value) {
    std::unique_lock lock(mu_);
    values_[std::move(key)] = std::move(value);
  }

private:
  mutable std::shared_mutex mu_;
  std::unordered_map<std::string, std::string> values_;
};
```

读操作只持有共享锁，多个线程可以同时执行 `get`；`set` 获取独占锁后会等待所有读者退出。`mu_` 加 `mutable` 是因为 `get` 是 `const` 成员函数，但仍需锁定互斥量。

## 真实项目证据

| 项目        | 位置                                                                                                                                                | 用法                                             | 为什么典型                                               |
| ----------- | --------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ | -------------------------------------------------------- |
| rui314/mold | [`src/output-chunks.cc#L2075-L2089`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/output-chunks.cc#L2075-L2089) | 共享锁查找输出段，未命中后用独占锁创建并二次检查 | 读多写少；拿到写锁后重新查找，避免并发插入重复对象       |
| rui314/mold | [`src/passes.cc#L571-L578`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/passes.cc#L571-L578)                   | 并行任务在共享锁下复制映射，再使用线程本地副本   | 临界区只做拷贝，后续查询不持锁；注释明确说明为降低锁竞争 |

两条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。mold 在 TBB 并行算法中使用该锁，说明共享锁与任务并行可以组合。

## 陷阱与边界

**读锁不保证数据不变。** `std::shared_lock` 只排斥写者；若多个读者同时修改同一数据，仍然存在数据竞争。共享锁适合“只读访问”，不适合“多个线程都写但希望并发”。

**没有原子的升级操作。** 不能把已持有的 `shared_lock` 直接提升为独占锁。需要写入时，先释放共享锁，再获取 `unique_lock`，并在获得写锁后重新检查条件：

```cpp
{
  std::shared_lock read_lock(mu);
  if (contains(key)) return;
}
std::unique_lock write_lock(mu);
if (contains(key)) return;
insert(key);
```

两段检查之间数据可能变化，第二次检查不可省略。

**写者可能饥饿。** 标准不规定读者与写者的优先级；读请求持续到来时，写线程可能长时间拿不到锁。写操作频繁或延迟敏感时，用公平性更好的实现，或在应用层限制读并发。

**临界区要短。** 读写锁的加解锁比普通 `mutex` 更重；只有读操作确实占多数、且临界区有一定工作量时才划算。单纯读一个 `int` 用 `std::atomic` 更便宜。

**不要递归加锁。** `shared_mutex` 不是递归互斥量；同一线程重复获取锁（无论共享还是独占）都可能死锁。需要递归语义时重新设计调用边界。

**生命周期仍然由调用方保证。** `shared_lock`、`unique_lock` 都只借用互斥量；锁对象不能比 `shared_mutex` 活得更久。

**条件变量需要区分类型。** `std::condition_variable` 只接受 `std::unique_lock<std::mutex>`；`std::condition_variable_any` 可以配合 `std::shared_lock` 和 `std::unique_lock` 使用，但等待/通知场景仍要重新检查条件，读多写少的缓存通常不需要它。不要为了读并发把普通等待队列强行换成读写锁。

## 横向对比

| 方案                      | 并发读 | 写独占     | 主要代价                         |
| ------------------------- | ------ | ---------- | -------------------------------- |
| `std::shared_mutex`       | 是     | 是         | 比 `mutex` 重；写者可能饥饿      |
| `std::mutex`              | 否     | 是         | 简单、开销低；读多写少时吞吐受限 |
| `std::shared_timed_mutex` | 是     | 是         | 支持超时；实现更重               |
| `std::atomic<T>`          | 是     | 是（单值） | 只适合单个可原子操作的值         |
| 线程本地缓存 + 定期发布   | 是     | 少量       | 需要处理快照一致性               |

跨标准演进：`std::shared_timed_mutex` 在 C++14 引入，`std::shared_mutex` 在 C++17 通过 N4508 加入，去掉了超时接口以获得更轻的实现。MSVC 从 19.0 Update 2（VS 2015 Update 2）起提供 `<shared_mutex>`；旧工具集上不能只按 `_MSC_VER >= 1900` 判断。C++20 的 `std::atomic<std::shared_ptr<T>>` 为“整体替换快照”提供了另一条路径，适合读多写少且写操作只做整体替换的场景。
