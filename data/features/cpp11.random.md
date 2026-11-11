---
id: cpp11.random
title: 随机数库（random）
standard: C++11
kind: library
category: [算法, 性能]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.5", clang: "3.0", msvc: "16.0"}
tags: [std::random, 随机数, mt19937, 分布]
alternatives: [rand, PCG, xorshift, absl::Random]
proposal: N1452
last_verified: "2026-11-06"
claims:
  - id: c1
    statement: "用均匀分布和线程局部 mt19937 生成随机字符串"
    evidence: [ev-drogon-random-string-001]
  - id: c2
    statement: "用 random_device 直接填充随机字节"
    evidence: [ev-mold-random-bytes-002]
  - id: c3
    statement: "用均匀分布随机选择工作窃取目标"
    evidence: [ev-stdexec-random-work-steal-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C 的 `rand()` 只有一个全局状态，质量因实现而异，`rand() % n` 还会引入取模偏差；把生成随机数和把它映射到某个范围混在一起，调用方很难替换质量更好的引擎。

C++11 的 `<random>` 把这两件事拆开：引擎（engine）负责产生均匀的原始比特流，分布（distribution）负责把它映射到整数、浮点或布尔范围。标准提供 `std::mt19937` 等可复现引擎，也提供 `std::random_device` 作为非确定性熵源。标准提案为 N1452。

## 基础用法

固定种子让输出可复现；`std::uniform_int_distribution` 把引擎输出映射到闭区间，`std::shuffle` 用同一个引擎打乱序列。

<!-- compile -->

```cpp
#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

int main() {
  // Fixed seed keeps the snapshot stable; production code seeds from
  // std::random_device.
  std::mt19937 rng(2024);

  std::uniform_int_distribution<int> dice(1, 6);
  std::vector<int> rolls;
  for (int i = 0; i < 6; ++i) {
    rolls.push_back(dice(rng));
  }
  std::sort(rolls.begin(), rolls.end());
  std::cout << "dice:";
  for (int value : rolls) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';

  std::vector<int> cards{1, 2, 3, 4, 5};
  std::shuffle(cards.begin(), cards.end(), rng);
  std::cout << "cards:";
  for (int value : cards) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';
}
```

输出 `dice: 2 4 4 5 5 5` 与 `cards: 3 1 5 4 2`。同一个种子和同一个标准库实现会得到相同序列；换实现时分布的具体映射可能不同。完整版本见 [`examples/cpp11/random.cpp`](../../examples/cpp11/random.cpp)，六个场景的输出为：

```text
[1] dice: 2 4 4 5 5 5
[2] in [0,1) = true
[3] coin = tails
[4] cards: 5 1 2 3 4
[5] reproducible = true
[6] seeded result in [1,100] = true
```

场景 1 用 `uniform_int_distribution` 生成六次骰子点数并排序；场景 2 换用浮点分布，断言结果落在 `[0, 1)` 内；场景 3 的 `bernoulli_distribution` 按概率产生布尔值；场景 4 用 `std::shuffle` 打乱序列，验证结果是元素集合的排列。场景 5 让两个独立引擎使用同一种子，第一次取值必然相同，这就是可复现的来源。场景 6 用 `random_device` 播种，但它的取值本身不可复现，因此只检查播种后的结果落在指定区间，避免把随机输出写进快照。

## 进阶用法

**引擎与分布分离。** `mt19937` 是 32 位梅森旋转引擎，周期长、质量稳定；`mt19937_64` 直接产生 64 位结果。需要更小状态或更快速度时，可以选择 `minstd_rand`、`ranlux` 或第三方 PCG。

**用 `random_device` 播种。** 生产代码通常写 `std::mt19937 rng(std::random_device{}());`，让每次运行得到不同序列；`random_device` 在部分平台上可能退化为确定性实现，不能把它当作密码学安全随机源。

**线程局部引擎。** 引擎有内部状态且不是线程安全的；多线程环境用 `thread_local` 引擎，或每个线程各自持有引擎，避免加锁和共享状态竞争。

**`seed_seq` 改善初始状态。** 直接把单个整数作为种子可能让前几个输出相关性偏强；`std::seed_seq` 可以把多个种子值扩散到整个状态。

**分布对象可复用。** 分布通常不持有状态（`std::normal_distribution` 除外），可以重复调用；不要在热循环里反复构造分布对象，除非编译器能完全优化掉。

## 真实项目证据

| 项目    | 位置                                                                                                                                                                                 | 用法                                 | 为什么典型                                                  |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------ | ----------------------------------------------------------- |
| drogon  | [`lib/src/Utilities.cc#L184-L198`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/src/Utilities.cc#L184-L198)                           | 均匀分布加线程局部引擎生成随机字符串 | 字符表长度决定分布范围，`thread_local` 避免每次调用重复播种 |
| mold    | [`lib/random.cc#L7-L18`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/random.cc#L7-L18)                                                          | `random_device` 直接填充随机字节     | 只需要不可预测的字节，不需要特定分布，入口选择最直接        |
| stdexec | [`include/exec/static_thread_pool.hpp#L1021-L1034`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/static_thread_pool.hpp#L1021-L1034) | 均匀分布随机选择工作窃取目标         | 从候选队列中随机挑选以分散竞争，范围随候选数量动态变化      |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖字符串生成、字节填充与并发调度三种用法。

## 陷阱与边界

**不要用 `rand() % n`。** 取模会把不均匀的余数映射到范围上，低值概率偏高；`uniform_int_distribution` 在实现内部处理拒绝采样，结果无偏。

**分布结果不保证跨实现一致。** 标准规定分布必须满足的统计性质，但没有规定具体的整数映射；同一个种子在 libstdc++ 与 MSVC STL 上可能得到不同序列。需要跨平台复现时，要么自己实现映射，要么只依赖引擎的原始输出。

**`random_device` 可能很慢或不确定。** 它每次调用可能访问系统熵源，适合播种而不适合大批量生成；某些平台上的实现是确定性的，不要用于安全令牌。

**引擎不是线程安全的。** 多个线程共享同一个引擎会产生数据竞争；用 `thread_local`、每线程独立引擎或外部加锁。

**固定种子只适合测试。** 生产环境用固定种子会让序列可预测；安全相关场景应使用操作系统提供的密码学随机源，而不是 `mt19937`。

**不要依赖 `default_random_engine`。** 它的具体类型由实现决定，跨标准库不可复现；需要稳定行为时显式写 `std::mt19937`。

## 库联动

- **`<random>`**：引擎、分布、`seed_seq` 与 `random_device` 都在这个头文件里。
- **`<algorithm>`**：`std::shuffle` 需要一个均匀随机位生成器，直接传入标准引擎即可。
- **`<chrono>`**：常见做法是用时间戳补充种子，但时间种子可预测，只适合非安全场景。
- **Abseil / PCG**：需要更快、更小状态或更强可复现性时，常用 `absl::Random` 或 PCG 系列引擎替换标准引擎。
- **drogon / mold / stdexec**：分别展示字符串生成、字节填充与并发任务调度中的随机数用法。

## 横向对比

| 方案                 | 状态大小  | 质量       | 可复现性     | 主要代价                 |
| -------------------- | --------- | ---------- | ------------ | ------------------------ |
| `std::rand`          | 全局      | 因实现而异 | 同实现可复现 | 范围映射有偏，线程不安全 |
| `std::mt19937`       | 约 2.5 KB | 高         | 标准规定算法 | 状态较大，生成速度中等   |
| PCG / xorshift       | 小        | 高         | 取决于实现   | 非标准库，需要自行引入   |
| `std::random_device` | 由实现定  | 非确定性   | 不可复现     | 调用开销高，质量依赖平台 |

跨标准演进：C++11 通过 N1452 引入 `<random>`，统一引擎与分布的接口；后续标准补充了更多分布和 `std::seed_seq` 的细节，但引擎/分布分离的设计没有改变。C++20 的 `std::span` 让批量填充随机字节的接口更容易表达。
