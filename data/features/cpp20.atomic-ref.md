---
id: cpp20.atomic-ref
title: std::atomic_ref
standard: C++20
kind: library
category: [并发, 原子操作]
difficulty: 4
status: final
tier: A
compilers: {gcc: "10", clang: "19", msvc: "19.28"}
tags: [atomic, 原子操作, 内存序, 共享内存, 无锁]
alternatives: [std::atomic, 互斥量, GCC __atomic 内建, 平台原子 API]
proposal: P0019R8
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "用特性测试宏检测 std::atomic_ref，不支持时回退到自有实现"
    evidence: [ev-stdexec-atomic-ref-guard-001]
  - id: c2
    statement: "把 mmap 得到的 io_uring 环形缓冲区指针绑定到 atomic_ref"
    evidence: [ev-stdexec-atomic-ref-submission-ring-002]
  - id: c3
    statement: "对同一块普通内存按 relaxed 与 acquire 内存序读取共享队列指针"
    evidence: [ev-stdexec-atomic-ref-load-store-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

`std::atomic<T>` 把原子操作做进了类型：要原子访问一个整数，就得把整数声明成 `std::atomic<int>`。但有些对象不能改类型——内核与用户态共享的 mmap 内存、C 库的结构体、按固定布局序列化的缓冲区、第三方接口传进来的普通指针。这些地方既不能把字段换成 `std::atomic`，又需要原子读写。

`std::atomic_ref<T>` 是对已有对象的原子引用。它不改变对象的类型和布局，只在对该对象操作时提供原子语义。代价是正确性责任转移到调用方：对齐、生命周期和“不得并发非原子访问”都需要自己保证。标准提案为 P0019R8。

## 基础用法

构造 `atomic_ref` 时传入已有对象，之后通过引用执行 `load`、`store`、`fetch_add`、`compare_exchange` 等操作。

<!-- compile -->

```cpp
#include <atomic>
#include <iostream>

int main() {
  unsigned value = 0;
  std::atomic_ref<unsigned> ref(value);

  ref.fetch_add(2);
  ref.fetch_add(3, std::memory_order_relaxed);
  ref.store(10, std::memory_order_release);

  std::cout << value << ' ' << ref.load(std::memory_order_acquire) << '\n';
}
```

输出为 `10 10`：普通变量 `value` 仍然是 `unsigned`，原子操作通过 `ref` 生效。完整版本见 [`examples/cpp20/atomic_ref.cpp`](../../examples/cpp20/atomic_ref.cpp)。

`T` 必须是可平凡复制的类型；被引用对象必须满足 `std::atomic_ref<T>::required_alignment`，并处于可写内存中。`atomic_ref` 只持有地址，不拥有对象，因此所有引用的生命周期都必须落在对象生命周期之内。

## 进阶用法

内存序与 `std::atomic` 一致：默认 `memory_order_seq_cst`，需要放松时显式传 `relaxed`、`acquire`、`release` 或 `acq_rel`。读改写操作可用 `fetch_add`、`fetch_sub`、`fetch_or`、`exchange`；比较交换有 `compare_exchange_weak` 与 `compare_exchange_strong`。

对共享数据结构，常见做法是每个需要原子访问的普通字段各建一个 `atomic_ref`，在构造时绑定到固定地址。stdexec 的 io_uring 后端就是如此：提交队列和完成队列的 `head`/`tail` 位于 mmap 内存中，用 `atomic_ref<__u32>` 绑定后，提交线程用 `release` 发布 `tail`，读取方按 `acquire` 观察；内核更新的 `head` 只做 `relaxed` 读取，因为同步由环形缓冲区的生产者/消费者协议保证。

`atomic_ref` 可以拷贝，拷贝出的引用指向同一个对象。多个 `atomic_ref` 之间的原子操作彼此原子；但只要还有 `atomic_ref` 引用某个对象，该对象就必须只通过这些 `atomic_ref` 访问，不能再同时走普通读写或 `std::atomic`。也不能让另一个 `atomic_ref` 并发引用它的子对象。

## 真实项目证据

| 项目    | 位置                                                                                                                                                                                     | 用法                                                                  | 为什么典型                                     |
| ------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------- | ---------------------------------------------- |
| stdexec | [`include/stdexec/__detail/__atomic.hpp#L68-L69`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__atomic.hpp#L68-L69)         | `#if __cpp_lib_atomic_ref >= 201806L` 后直接 `using std::atomic_ref`  | 跨工具链使用前的特性检测与回退                 |
| stdexec | [`include/exec/linux/io_uring_context.hpp#L217-L226`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/linux/io_uring_context.hpp#L217-L226) | 用 mmap 区域中的 `sq_off.head`/`sq_off.tail` 构造 `atomic_ref<__u32>` | 内核共享内存布局不能改类型，只能原地加原子语义 |
| stdexec | [`include/exec/linux/io_uring_context.hpp#L239-L240`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/linux/io_uring_context.hpp#L239-L240) | 对 `tail` 做 relaxed 读、对 `head` 做 acquire 读                      | 生产者/消费者协议需要逐字段选择内存序          |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。第一条是生产头文件的兼容层，后两条来自 Linux io_uring 执行器；同一文件第 276 行用 release 存储发布 `tail`，与第 239-240 行构成完整的发布/获取配对。

## 陷阱与边界

**并发非原子访问是数据竞争。** `atomic_ref` 只让经过它的访问具有原子性。`atomic_ref` 存在期间，对象必须只通过 `atomic_ref` 访问；另一个线程直接读写、或改用 `std::atomic`，都破坏这条排他约定。也不要让另一个 `atomic_ref` 并发引用同一对象的子对象。

**对齐要求可能比 `T` 本身更严格。** 对象必须对齐到 `std::atomic_ref<T>::required_alignment`。从外部缓冲区取地址时，用 `alignas(std::atomic_ref<T>)` 或运行时检查对齐，不能假设 `T` 的自然对齐已经足够。

**生命周期由调用方保证。** `atomic_ref` 不延长对象寿命。对已经释放的对象、栈上已经离开作用域的变量做原子操作，都是悬空访问。

**不保证无锁。** 用 `is_lock_free()` 或 `is_always_lock_free` 检查；某些平台上原子操作可能退化为内部锁，实时性或信号安全场景要单独验证。

**不能用于位域和只读存储。** 位域没有独立地址；需要写入的对象必须位于可写内存，不能指向字符串字面量或只读段。

**共享内存的布局约束仍然存在。** `atomic_ref` 不改变布局，但要求另一端也按相同的类型、对齐和内存序访问。跨进程或跨内核边界时，这些约定必须写进协议，而不是指望类型系统。

## 库联动

- **`std::atomic` 与 `std::memory_order`**：语义完全一致，`atomic_ref` 只是把同一组操作应用到外部对象上。
- **io_uring / mmap**：环缓冲区的 head/tail 是固定布局的普通整数，`atomic_ref` 是在不改变 ABI 的前提下加原子性的直接工具。
- **无锁数据结构**：节点中由外部分配、布局固定的字段可用 `atomic_ref` 接入无锁算法。
- **OS 与设备共享缓冲区**：DMA 或共享内存区域通常由驱动定义布局，只能通过引用式原子操作访问。
- **线程检查工具**：ThreadSanitizer 等工具对原子操作有专门标注；绕过它们直接读写同一对象会留下无法解释的竞争报告。

## 横向对比

| 方案                 | 改变对象类型 | 可用于固定布局内存 | 生命周期责任 | 主要代价                                   |
| -------------------- | ------------ | ------------------ | ------------ | ------------------------------------------ |
| `std::atomic<T>`     | 是           | 否                 | 对象自身     | 无法接入外部定义的布局                     |
| `std::atomic_ref<T>` | 否           | 是                 | 调用方       | 对齐、生命周期与并发访问约束               |
| 互斥量               | 否           | 需要额外锁对象     | 调用方       | 阻塞、需要共享锁，不能用于信号或中断上下文 |
| GCC `__atomic` 内建  | 否           | 是                 | 调用方       | 不可移植，需手写内存序与类型处理           |

跨标准演进：C++20 通过 P0019R8 引入 `std::atomic_ref`；它填补了“对象布局不能改，但仍需原子访问”的场景，而不是替代 `std::atomic`。
