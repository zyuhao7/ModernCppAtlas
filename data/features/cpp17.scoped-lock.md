---
id: cpp17.scoped-lock
title: std::scoped_lock
standard: C++17
kind: library
category: [并发, RAII, 线程同步]
difficulty: 3
status: final
tier: A
compilers: {gcc: "7", clang: "5", msvc: "19.11"}
tags: [互斥量, 多锁, 死锁避免, RAII]
alternatives: [std::lock_guard, std::unique_lock, std::lock, 手动按序加锁]
proposal: P0156R2
last_verified: "2026-10-25"
claims:
  - id: c1
    statement: "用 scoped_lock 保护一次性输出，并在静态成员上直接声明 inline mutex"
    evidence: [ev-mold-scoped-lock-emit-001]
  - id: c2
    statement: "在 tbb::parallel_for_each 的任务体内用 scoped_lock 保护共享符号状态"
    evidence: [ev-mold-scoped-lock-parallel-export-003]
  - id: c3
    statement: "在并行循环中按符号加锁，保护多字段的一致性更新"
    evidence: [ev-mold-scoped-lock-symbol-merge-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

`std::lock_guard` 只能锁一个互斥量。需要同时锁住两个对象时，常见写法是手动调用 `m1.lock(); m2.lock();`，再在每条退出路径上配对解锁。两个线程若以相反顺序加锁，就会互相等待；异常路径还可能漏解锁。

`std::scoped_lock` 把“作用域结束时释放”和“多锁死锁避免”合到一个 RAII 类型里。构造时对多个互斥量执行与 `std::lock` 等价的死锁避免加锁，析构时释放已持有的锁。单参数形式可以替代 `lock_guard`，多参数形式则消除了手写加锁顺序这一整类错误。

## 基础用法

<!-- compile -->

```cpp
#include <mutex>

struct Account {
  int balance;
  std::mutex mu;
};

void transfer(Account &from, Account &to, int amount) {
  std::scoped_lock lock(from.mu, to.mu);
  if (from.balance < amount) return;
  from.balance -= amount;
  to.balance += amount;
}
```

完整示例见 [`examples/cpp17/scoped_lock.cpp`](../../examples/cpp17/scoped_lock.cpp)：

```text
total 2000
a 800 b 1200
unchanged 800 1200
```

示例启动 100 个线程，分别从 `a` 到 `b` 与从 `b` 到 `a` 转账；两个方向的调用都同时锁住两个账户。若换成固定顺序的 `lock_guard`，两个方向的代码必须写成同一顺序才不会死锁。最后一条演示余额不足时在锁内检查并直接返回。

## 进阶用法

**类模板实参推导省掉类型。** C++17 之前写 `std::lock_guard<std::mutex> lock(mu);`；现在可以写 `std::scoped_lock lock(mu);` 或 `std::scoped_lock lock(mu1, mu2);`，模板参数由构造函数实参推导。

**多锁算法不是“按参数顺序依次加锁”。** 标准只要求避免死锁，不规定具体次序；常见实现逐个尝试 `try_lock`，失败就释放已持有的锁并重试。因此调用方传入互斥量的顺序不同也不会形成环等待。它保证不死锁，但不保证公平，也不保证不重试。

**已持有的锁用 `adopt_lock` 接管。** 如果互斥量已经由当前线程锁定，可以写 `std::scoped_lock lock(std::adopt_lock, mu);`，构造时不再加锁，析构时仍会解锁。标签参数在前，与 `lock_guard` 把标签放在后面的形式不同。只在确实已经持锁时使用；对未持有的互斥量使用是未定义行为。

**没有 `defer_lock`。** `scoped_lock` 的目标就是“构造即加锁、析构即解锁”。需要先构造后加锁、提前解锁或把锁移交给别的对象时，用 `std::unique_lock`。

**单个互斥量时与 `lock_guard` 等价。** 两者都在作用域结束时解锁；`scoped_lock` 的优势是统一的写法与 CTAD。已有代码里的 `lock_guard` 不必为了风格统一而批量替换。

**同一个互斥量不能在一次构造中重复传入。** `std::scoped_lock lock(mu, mu);` 违反前置条件；即使使用 `std::recursive_mutex` 也不应依赖重复计数。需要嵌套获取时，重新审视锁的边界。

**锁的粒度仍然是设计问题。** `scoped_lock` 只保证正确释放，不会缩短临界区。格式化、I/O、内存分配等慢操作应移到锁外；并行循环里按对象加锁，而不是用一把全局锁把循环串行化。

**与读写锁、条件变量分工。** `scoped_lock` 只能以独占方式锁定 `std::shared_mutex`；只读共享需要 `std::shared_lock`。`std::condition_variable::wait` 要求 `std::unique_lock`，因为它需要在等待期间解锁并在唤醒后重新加锁。

## 真实项目证据

| 项目        | 位置                                                                                                                                        | 用法                                                  | 为什么典型                                                    |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------- | ------------------------------------------------------------- |
| rui314/mold | [`lib/common.h#L100-L114`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/common.h#L100-L114)             | 单参数锁定一次性输出，配合 `static inline std::mutex` | 输出与 `emitted` 标志作为整体更新；提前 `return` 也不会漏解锁 |
| rui314/mold | [`src/relocatable.cc#L121-L133`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/relocatable.cc#L121-L133) | 并行循环中按符号加锁，保护多字段更新                  | 检查与写入处于同一临界区，锁在循环迭代结束时自动释放          |
| rui314/mold | [`src/passes.cc#L1947-L1955`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/passes.cc#L1947-L1955)       | 在 `tbb::parallel_for_each` 任务体内锁定单个符号      | 并行算法不消除数据竞争；按元素加锁避免全局串行化              |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。mold 使用 TBB 并行算法，并在每个 `Symbol` 内维护独立的 `mu`。

## 陷阱与边界

- **不要把它当成“性能优化开关”。** 多锁算法在竞争激烈时可能反复重试；真正减少竞争要靠缩小临界区、分片或改变数据归属。
- **锁顺序问题被消除，不代表共享数据设计正确。** 忘记加锁、锁错对象、读写不在同一临界区，`scoped_lock` 都帮不上。
- **提前解锁做不到。** 需要“锁住初始化、解锁后继续计算”时，用 `unique_lock` 并显式调用 `unlock()`。
- **不能移动或复制。** `scoped_lock` 的所有权固定在声明作用域；需要转移锁所有权时用 `unique_lock`。
- **在构造函数里加锁并暴露 `this` 仍有风险。** 对象尚未完全构造就被其他线程看到，是独立于锁类型的问题；共享对象应在构造完成后再发布。
- **异常会触发解锁，但不会回滚数据。** 若临界区内修改了多个字段后抛出异常，锁会释放，已做的部分修改仍然存在。
- **同一线程重复加锁非递归互斥量是未定义行为。** 即使换用 `std::recursive_mutex`，也常常说明调用链的锁边界不清晰。
- **锁持有期间不要调用未知回调。** 回调可能再次进入同一路径或长时间阻塞；把回调移到锁外执行。

## 库联动

- **mold**：在 TBB 的 `parallel_for_each` 内部按 `Symbol` 加锁，把并行扫描与细粒度同步组合起来；`lib/common.h` 用 `static inline std::mutex` 在头文件内定义共享互斥量。
- **spdlog**：日志后端通常用互斥量保护 sink 的写入；格式化在加锁前完成可以缩短临界区，`scoped_lock` 适合这种固定作用域的写法。
- **fmt**：格式化本身不持有业务锁；推荐的模式是先在锁外生成字符串，再在锁内写入共享缓冲区，避免把 `fmt::format` 放进临界区。
- **TBB / 并行算法**：并行任务仍然需要显式同步；按元素或按分片加锁通常优于一把全局锁。
- **条件变量与读写锁**：等待条件用 `unique_lock`，只读共享用 `shared_lock`；`scoped_lock` 负责的是固定作用域内的独占锁定。

## 横向对比

| 方案                            | 支持多锁 | 死锁避免                        | 可提前解锁 | 主要代价                             |
| ------------------------------- | -------- | ------------------------------- | ---------- | ------------------------------------ |
| `std::scoped_lock`              | 是       | 是，构造时使用 `std::lock` 算法 | 否         | 多锁时可能重试；类型不可移动         |
| `std::lock_guard`               | 否       | 不适用                          | 否         | 多锁需手写顺序与配对解锁             |
| `std::unique_lock`              | 单个     | 否                              | 是         | 状态更多，略重于 `lock_guard`        |
| `std::lock` + 两个 `lock_guard` | 是       | 是                              | 否         | 需要 `adopt_lock` 正确接管，写法更长 |
| 手动 `lock` / `unlock`          | 是       | 否                              | 是         | 异常与提前返回路径容易漏解锁         |

跨标准演进：C++17 引入 `std::scoped_lock`（P0156R2），同时支持类模板实参推导；此后接口保持稳定。需要 C++11/14 兼容时，用 `std::lock` 加 `std::lock_guard` 与 `std::adopt_lock` 组合获得同样的死锁避免语义。
