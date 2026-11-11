---
id: cpp11.chrono
title: std::chrono
standard: C++11
kind: library
category: [性能]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "17.0"}
tags: [chrono, duration, time_point, steady_clock, 计时]
alternatives: [C time API, 平台计时器, 纳秒时间戳, 第三方日期库]
proposal: N2661
last_verified: "2026-11-03"
claims:
  - id: c1
    statement: "用 steady_clock 和 duration<double> 实现秒级、毫秒级计时器"
    evidence: [ev-spdlog-chrono-stopwatch-001]
  - id: c2
    statement: "用 chrono::milliseconds 限定条件变量的等待时长"
    evidence: [ev-spdlog-chrono-wait-for-002]
  - id: c3
    statement: "用 steady_clock::now 驱动定时任务，并用 wait_until 等待下一个截止点"
    evidence: [ev-stdexec-chrono-steady-clock-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C 的 `time.h` 用 `time_t` 表示秒级时间戳，用 `clock()` 表示处理器时间，单位靠注释和命名约定传递。需要毫秒、微秒或纳秒时，代码里到处都是乘以 1000 的魔数；不同类型的时长可以互相赋值而不报错，混用单位的问题往往到运行期才暴露。

`<chrono>` 把“时长”和“时间点”做成带单位的类型：`duration<Rep, Period>` 在类型里携带单位，`time_point<Clock, Duration>` 在类型里携带时钟，`duration_cast` 负责显式换算。标准提案为 N2661。

## 基础用法

`std::chrono::duration` 由计数类型和单位比例组成；标准库提供 `nanoseconds`、`microseconds`、`milliseconds`、`seconds`、`minutes`、`hours` 等别名。`time_point` 表示某个时钟上的时刻，两个时间点相减得到 `duration`。不同单位之间的隐式转换只在不会丢失精度时发生，其余情况必须用 `duration_cast`。

<!-- compile -->

```cpp
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  using namespace std::chrono;

  milliseconds interval{1500};
  seconds rounded = duration_cast<seconds>(interval);
  std::cout << interval.count() << ' ' << rounded.count() << '\n';

  duration<double> as_seconds = interval;
  std::cout << as_seconds.count() << '\n';

  minutes minute{1};
  std::cout << duration_cast<milliseconds>(minute).count() << '\n';

  steady_clock::time_point start = steady_clock::now();
  std::this_thread::sleep_for(milliseconds(2));
  steady_clock::time_point end = steady_clock::now();
  const auto elapsed = duration_cast<milliseconds>(end - start);
  std::cout << (elapsed >= milliseconds::zero()) << '\n';

  const auto deadline = start + seconds(1);
  std::cout << (deadline > end) << '\n';
}
```

输出 `1500 1`、`1.5`、`60000`、`1` 与 `1`。示例先展示毫秒到秒的显式截断和到 `duration<double>` 的精度保持，再用 `steady_clock` 测量一次短暂休眠，最后演示时间点与时长相加得到截止时刻。完整版本见 [`examples/cpp11/chrono.cpp`](../../examples/cpp11/chrono.cpp)，五个场景的输出为：

```text
[1] ms = 1500, cast to s = 1, as double s = 1.5
[2] minute in s = 60, in ms = 60000
[3] 750ms / 1s = 0.75
[4] slept at least 2ms = true
[5] deadline after end = true, remaining s = 0
```

场景 1 对比两种转换：`duration_cast<seconds>` 截断得到 1，赋给 `duration<double>` 保留 1.5。场景 2 是反向的隐式转换：`minutes` 赋给 `seconds` 不会丢失精度，因此不需要 `duration_cast`，显式转换到毫秒才得到 60000。场景 3 演示时长相除得到无量纲比值；整数计数相除会截断，所以先把计数换成 `double`。场景 4 用 `steady_clock` 测量 2ms 休眠，并断言耗时不少于 2ms。场景 5 把截止时刻与当前时刻比较，再用 `duration_cast` 截断剩余时间。

`count()` 返回底层计数，不带单位；打印或跨接口传递裸数字会丢掉单位信息。需要固定单位时，先用 `duration_cast` 转成目标类型再取 `count()`。

## 进阶用法

`duration` 支持加减乘除：两个时长相加得到共同单位，时长乘除标量保持单位，两个时长相除得到无量纲比值。`time_point` 支持加减 `duration`，两个同钟时间点相减得到时长；不同时钟的时间点不能直接比较。

时钟选择决定语义。`system_clock` 对应墙上时间，可转换为 `time_t`，但可能因校时向后跳；`steady_clock` 单调递增，适合测量耗时和实现超时；`high_resolution_clock` 通常是前两者的别名，不保证单调性。

有条件等待优先用 `wait_until` 配合绝对截止点，而不是循环 `wait_for` 累加，后者会因每次唤醒的调度延迟逐渐漂移。把截止点设为 `now + timeout` 后交给条件变量，可以避免忙等并保持超时语义。

C++14 起提供 `chrono_literals`：`1500ms`、`2s`、`1min` 等字面量让单位直接出现在源码里。C++20 又加入日历、时区和 `std::formatter` 支持，但核心的 duration/time_point 模型自 C++11 起未变。

## 真实项目证据

| 项目    | 位置                                                                                                                                                                                     | 用法                                                    | 为什么典型                                         |
| ------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------- | -------------------------------------------------- |
| spdlog  | [`include/spdlog/stopwatch.h#L30-L44`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/stopwatch.h#L30-L44)                                | 用 `steady_clock` 和 `duration_cast` 实现秒/毫秒计时    | 单调时钟测耗时，两个单位接口并列展示隐式与显式转换 |
| spdlog  | [`include/spdlog/details/mpmc_blocking_q.h#L68-L79`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/mpmc_blocking_q.h#L68-L79)    | 用 `std::chrono::milliseconds` 作为阻塞队列的等待上限   | 超时类型进入接口签名，调用方无法误传裸数字         |
| stdexec | [`include/exec/timed_thread_scheduler.hpp#L175-L185`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/timed_thread_scheduler.hpp#L175-L185) | 用 `steady_clock::now` 与 `wait_until` 驱动定时任务调度 | 时间点比较、时长运算与条件变量等待组合成事件循环   |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖耗时测量、有界等待与定时调度三种场景。

## 陷阱与边界

**`duration_cast` 会截断。** 从毫秒转到秒时 `1500ms` 变成 `1s`，小数部分直接丢弃；需要四舍五入时用 `round` 或自行处理余数。

**`system_clock` 不单调。** 系统校时或时区调整可能让它回退，测量耗时或实现超时必须用 `steady_clock`。

**`high_resolution_clock` 不保证单调。** 它通常是 `system_clock` 或 `steady_clock` 的别名，不能仅凭名字判断语义。

**不同时钟的时间点不能混用。** `system_clock::time_point` 与 `steady_clock::time_point` 是不同类型，之间的转换没有标准语义。

**`count()` 返回裸数字。** 直接打印或存进整数字段会丢失单位，跨模块传递时优先保留 `duration` 类型。

**字面量需要命名空间。** `1500ms` 依赖 `std::chrono_literals`，C++11 下不可用；C++14 起才进入标准。

**测量本身有开销。** 高频调用 `now()` 会扰动被测代码；微基准测试要把计时开销和优化屏障一起考虑。

## 库联动

- **`std::thread`**：`sleep_for` 接受 `duration`，`sleep_until` 接受 `time_point`，不需要手工换算成毫秒。
- **`std::condition_variable`**：`wait_for` / `wait_until` 用 chrono 类型表达超时，避免整数单位歧义。
- **`std::filesystem`**：`file_time_type` 基于 `std::chrono::time_point`，文件时间可以直接做差和排序。
- **spdlog**：`stopwatch` 与日志时间戳都建立在 chrono 上，提供秒和毫秒两种 elapsed 接口。
- **stdexec / 定时器库**：定时调度器用 `steady_clock` 排序任务，并用 `wait_until` 等待下一个截止点。

## 横向对比

| 方案          | 单位安全     | 单调时钟 | 超时接口   | 主要代价                           |
| ------------- | ------------ | -------- | ---------- | ---------------------------------- |
| `std::chrono` | 类型携带单位 | 可选择   | 原生支持   | 类型名较长，需要显式转换           |
| C `time.h`    | 否，靠约定   | 否       | 需自行实现 | 秒级精度，跨平台行为不一致         |
| 平台计时器    | 否           | 通常单调 | 平台相关   | 不可移植，需条件编译               |
| 第三方日期库  | 是           | 是       | 是         | 引入依赖，C++20 后部分功能已标准化 |

跨标准演进：C++11 通过 N2661 引入 duration、time_point 与三种时钟；C++14 增加 chrono 字面量；C++20 加入日历、时区与格式化支持。核心的“单位进类型、换算显式化”原则从 C++11 一直保持。
