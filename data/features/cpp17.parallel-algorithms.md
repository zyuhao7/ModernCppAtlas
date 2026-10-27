---
id: cpp17.parallel-algorithms
title: 并行算法与执行策略
standard: C++17
kind: library
category: [算法, 并发]
difficulty: 3
status: final
tier: A
compilers: {gcc: "9", clang: "17", msvc: "19.14"}
tags: [execution, par, par_unseq, 并行算法, STL]
alternatives: [OpenMP, Intel TBB, 手写线程池切分, 用 std::thread 直接分片]
proposal: P0024R2
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用 std::execution::par 并行遍历文件列表，共享容器由互斥量保护"
    evidence: [ev-potree-parallel-policy-paths-001]
  - id: c2
    statement: "把执行策略存进局部变量再传给 for_each，便于集中切换顺序与并行"
    evidence: [ev-potree-parallel-policy-scale-002]
  - id: c3
    statement: "用 par_unseq 并行排序点云，比较器只读入参、不碰共享状态"
    evidence: [ev-potree-parallel-policy-unseq-sort-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

标准库算法原本只有一条执行路径：在当前线程上按顺序处理整个区间。要在多核算力上摊开工作，过去只能引入 OpenMP 指令、TBB 的 `parallel_for`，或自己把区间切块再交给线程池。这些方案各有自己的任务调度和异常规则，换一个编译器就要重写一遍。

执行策略把“怎么执行”从算法实现里拆出来。`std::sort`、`std::for_each`、`std::transform`、`std::reduce` 等算法增加接受策略参数的重载，同一次调用可以在 `std::execution::seq`（顺序）、`par`（并行）、`par_unseq`（并行且向量化）之间切换。标准提案为 P0024R2，头文件是 `<execution>`。

## 基础用法

策略是算法调用的第一个参数，含义是允许实现自行决定并行粒度，而不是强制它开线程。

<!-- compile -->

```cpp
#include <algorithm>
#include <execution>
#include <iostream>
#include <numeric>
#include <vector>

int main() {
  std::vector<int> values{5, 3, 1, 4, 2};

  std::sort(std::execution::par, values.begin(), values.end());
  const int sum = std::reduce(std::execution::par, values.begin(), values.end());

  std::cout << values.front() << ' ' << values.back() << ' ' << sum << '\n';
}
```

输出 `1 5 15`。`reduce` 不保证按左结合顺序累加，因此它的初值和运算符必须满足结合律与交换律；需要固定顺序时仍用 `std::accumulate`。完整版本见 [`examples/cpp17/parallel_algorithms.cpp`](../../examples/cpp17/parallel_algorithms.cpp)。

`seq` 明确要求顺序执行且不向量化；`par` 允许跨线程并行；`par_unseq` 进一步允许在同一线程内交错、向量化执行。不传策略的旧重载行为等价于 `seq`。

## 进阶用法

并行算法处理的是“互相独立的迭代”。用 `par` 遍历时，闭包会被多个线程同时调用，所有共享状态都要自己同步：计数器用原子量或互斥量，写入不同元素时确认下标互不重叠，容器扩容必须在进入算法之前完成。

`par_unseq` 的约束更严。它的调用可以交错执行甚至在同一条指令里成组执行，闭包中不能有互斥锁、不能分配可能阻塞的内存、不能调用非可重入的库函数。比较器、投影函数只读取自己的实参时才是安全的。

异常处理与顺序版本不同。元素访问函数抛出的异常会被捕获，实现随后调用 `std::terminate`，而不是把异常传播给调用者。需要逐元素报告失败时，应在闭包内记录错误，让算法正常返回后再统一处理。

迭代器要求也比顺序算法更严：并行重载通常要求前向迭代器以上，部分算法要求随机访问迭代器。链表容器不能直接套用 `par` 版本的 `sort`。

执行策略是“许可”而非“保证”。实现可以选择忽略策略、退回串行执行，标准允许这种降级，所以不能把**并行**当作正确性前提，例如依赖执行顺序或线程数。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                     | 用法                                                                   | 为什么典型                                          |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------- | --------------------------------------------------- |
| Potree | [`Converter/src/main.cpp#L206-L208`](https://github.com/potree/PotreeConverter/blob/c2cb61843bf2b901e8e257874b1a6dcf67792fb2/Converter/src/main.cpp#L206-L208)                           | 用 `std::execution::par` 并行读取多个输入文件，结果容器由 `mutex` 保护 | 文件之间天然独立，共享写入仍要串行化                |
| Potree | [`Converter/include/PotreeConverter.h#L213-L216`](https://github.com/potree/PotreeConverter/blob/c2cb61843bf2b901e8e257874b1a6dcf67792fb2/Converter/include/PotreeConverter.h#L213-L216) | 把策略存到局部变量 `parallel`，再用它遍历所有数据源                    | 策略集中管理，调试点只需改一处即可退回顺序执行      |
| Potree | [`Converter/include/sampler_poisson.h#L183-L184`](https://github.com/potree/PotreeConverter/blob/c2cb61843bf2b901e8e257874b1a6dcf67792fb2/Converter/include/sampler_poisson.h#L183-L184) | 用 `par_unseq` 按到中心的距离排序点云，比较器只读两个 `Point` 实参     | 比较器无共享状态，符合 unsequenced 策略的可重入要求 |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都是生产路径，其中两处把策略先存成变量，说明工程代码更愿意在同一位置统一开关并行。

## 陷阱与边界

**并行不等于更快。** 小数据集上调度开销可能超过计算收益。元素很少、每个元素工作量很轻时，`seq` 往往更快，是否启用应当实测。

**共享状态必须自己同步。** 算法只并行化迭代，不会替你加锁。多个线程同时 `push_back` 到同一容器、同时递增普通 `int`，都是数据竞争。

**异常会终止进程。** 并行重载捕获到异常后调用 `std::terminate`，不会沿调用栈抛出。需要错误处理时，在闭包里记录状态，返回后再检查。

**`par_unseq` 的闭包要可重入。** 加锁、分配、I/O 以及依赖线程局部状态的操作都不适合放在 `par_unseq` 的回调里。

**迭代器类别有限制。** 并行重载通常要求前向或随机访问迭代器；`std::list`、输入流迭代器不能使用 `par`。

**实现可能串行执行。** 标准只要求“允许”并行，不保证真的并行。依赖线程数、执行顺序或真正并发的测试会在不同标准库上失败。

**标准库支持参差。** GCC 9 起提供并行算法，但底层使用自己的线程池；Clang 需要较新版本且依赖 TBB；部分平台上的 `par` 完全退化为串行。

## 库联动

- **标准算法族**：`for_each`、`transform`、`reduce`、`sort`、`remove_if`、`count_if` 等都有执行策略重载，迁移成本低于换框架。
- **Intel TBB**：libstdc++ 与 libc++ 的并行后端通常基于 TBB 调度器，已引入 TBB 的项目可以直接受益。
- **OpenMP**：适合需要精细控制线程绑定、调度策略或设备卸载的场景；执行策略没有这些旋钮。
- **线程池与任务库**：自带线程池的项目（如 stdexec、asio）通常更愿意直接用任务图，而不是标准并行算法。

## 横向对比

| 方案           | 侵入性                 | 能否向量化       | 异常语义             | 主要代价                     |
| -------------- | ---------------------- | ---------------- | -------------------- | ---------------------------- |
| 执行策略       | 低，改一个参数         | `par_unseq` 允许 | 抛异常即 `terminate` | 依赖标准库后端，调优空间小   |
| OpenMP         | 中，需要编译指令       | 取决于编译器     | 循环内异常受限       | 编译器相关，库边界要单独处理 |
| Intel TBB      | 高，需改写为任务或区间 | 部分             | 任务内异常可传播     | 引入第三方依赖与学习成本     |
| 手写线程池分片 | 高，需管理分片与汇合   | 否               | 完全自控             | 代码量大，容易写出数据竞争   |

跨标准演进：C++17 通过 P0024R2 把执行策略与并行算法纳入标准库；C++20 又为 `std::ranges` 补充了对应的约束式算法，但执行策略仍属于经典迭代器接口。
