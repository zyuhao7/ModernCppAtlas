---
id: cpp20.chrono-calendar
title: std::chrono 日历类型
standard: C++20
kind: library
category: [格式化, I/O]
difficulty: 3
status: final
tier: A
compilers: {gcc: "11", clang: "19", msvc: "19.29"}
tags: [chrono, 日历, 日期, year_month_day, 时间日期]
alternatives: [std::tm, 手写日期计算, 平台日期 API, Howard Hinnant date 库]
proposal: P0355R7
last_verified: "2026-11-18"
claims:
  - id: c1
    statement: "把时间点落到天后构造 year_month_day，再取出年月日格式化 ISO 8601"
    evidence: [ev-nghttp2-chrono-calendar-iso8601-001]
  - id: c2
    statement: "把 time_point floor 到 days 后构造 year_month_day 取年、月、日字段"
    evidence: [ev-glaze-chrono-calendar-ymd-002]
  - id: c3
    statement: "格式化库为 year_month_day 提供 formatter，把年月日映射到 std::tm"
    evidence: [ev-fmt-chrono-calendar-ymd-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

在 C++20 之前，日期计算主要依赖 `std::tm` 和 `time_t`：`tm` 的字段是普通整数，月份从 0 开始，年份从 1900 开始；闰年、月份天数和星期都要自己算，稍不留神就出现差一错误。把时间点拆成年、月、日，或者判断某个日期是否合法，往往要借助平台函数或第三方日期库。

C++20 的 `<chrono>` 日历把日期做进类型：`year_month_day` 表示年月日，`year_month_day_last` 表示某月最后一天，`weekday` 表示星期，`sys_days` 表示从纪元起的天数。它们可以互相转换，并带有 `ok()` 校验与闰年判断。P0355R7 把日历与时间带加入标准，让日期运算有了可推导、可比较、可格式化的类型。

## 基础用法

构造 `year_month_day` 后，通过 `year()`、`month()`、`day()` 取字段；与 `sys_days` 互转可以拿到自 1970-01-01 起的天数。

<!-- compile -->

```cpp
#include <chrono>
#include <iostream>

int main() {
  using namespace std::chrono;
  const year_month_day date{year{2024}, month{3}, day{14}};
  std::cout << int(date.year()) << ' ' << unsigned(date.month()) << ' '
            << unsigned(date.day()) << '\n';
}
```

输出 `2024 3 14`。完整版本见 [`examples/cpp20/chrono_calendar.cpp`](../../examples/cpp20/chrono_calendar.cpp)，六个场景的输出为：

```text
[1] 2024-03-14
[2] weekday = 4
[3] days since epoch = 19796
[4] last day of Feb 2024 = 29
[5] 2023-02-29 ok = false
[6] 2024 leap = true, 2023 leap = false
```

场景 1 构造 2024 年 3 月 14 日并输出年月日。场景 2 把它转成 `sys_days` 再构造 `weekday`，`c_encoding()` 以周日为 0，2024-03-14 是周四，输出 4。场景 3 输出自纪元起的天数 19796。场景 4 用 `year_month_day_last` 取闰年二月最后一天，得到 29。场景 5 构造不存在的 2023-02-29，`ok()` 返回 false。场景 6 用 `year::is_leap()` 判断闰年。

`year_month_day` 不会在构造时抛异常或夹取；非法日期照样保留，由 `ok()` 报告。`month`、`day`、`year` 都是强类型，`unsigned(month)` 和 `int(year)` 用于取数值，直接输出会走各自的 `operator<<`。

## 进阶用法

**先 floor 到天，再拆分字段。** 从 `system_clock::time_point` 取年月日时，先把时间点 floor 到 `days`，再用 `sys_days` 构造 `year_month_day`。nghttp2 的 `format_iso8601` 在拿到本地时间后正是这样做的：`floor<days>` 去掉时分秒，`year_month_day{days}` 再拆出日期字段，避免手写历法换算。

**带时区要先转本地时间。** `zoned_time{tz, tp}` 把系统时间映射到指定时区，`get_local_time()` 得到该时区的墙上时间，之后才能 floor 到天。直接对 `system_clock::time_point` floor 得到的是 UTC 日期，跨时区格式化会差一天。

**序列化只需三个字段。** glaze 的 ISO 时间输出把 `time_point` floor 到 `days`，构造 `year_month_day`，再取 `year()`、`month()`、`day()` 写进缓冲区。标准类型负责历法，序列化代码只负责排版。

**格式化库直接消费它。** fmt 为 `year_month_day` 提供 `formatter` 特化，把年、月、日映射到 `std::tm` 的 `tm_year`、`tm_mon`、`tm_mday`（分别减去 1900 和 1）。这说明 `year_month_day` 已成为跨库传递日期的通用载体。

**月末与星期可以直接查询。** `year_month_day_last` 表示某月最后一天，`weekday` 可以从 `sys_days` 构造；`year_month_day` 与 `sys_days` 之间可以隐式或显式互转，适合做日期加减与区间计算。

## 真实项目证据

| 项目               | 位置                                                                                                                                                                     | 用法                                                                     | 为什么典型                                                 |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------ | ---------------------------------------------------------- |
| nghttp2/nghttp2    | [`src/util.cc#L208-L222`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/util.cc#L208-L222)                                        | 用 `zoned_time` 取本地时间，floor 到天后构造 `year_month_day` 拆分年月日 | 生产路径的 ISO 8601 格式化，展示时间点转日历字段的完整流程 |
| stephenberry/glaze | [`include/glaze/core/chrono.hpp#L377-L381`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/core/chrono.hpp#L377-L381) | `floor<days>` 后构造 `year_month_day`，取年、月、日写序列化缓冲          | JSON 序列化库复用标准日历类型，不再手写 epoch 天数换算     |
| fmtlib/fmt         | [`include/fmt/chrono.h#L2021-L2028`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/chrono.h#L2021-L2028)                       | 为 `year_month_day` 提供 `formatter`，把字段映射到 `std::tm`             | 格式化库直接消费标准类型，说明它已成为日期传递的通用载体   |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径，覆盖时间点转换、二进制/文本序列化与格式化三种场景。

## 陷阱与边界

**非法日期不会自动纠正。** `year_month_day{year{2023}, month{2}, day{29}}` 可以构造成功，但 `ok()` 为 false；使用前应显式检查。

**月份与年份的数值类型。** `month` 和 `day` 是强类型，`unsigned(month)`、`int(year())` 才能取出整数；`day` 与 `unsigned` 转换不会做范围检查。

**floor 到天的时区语义。** 对 UTC 时间点直接 floor 得到 UTC 日期；需要本地日期时必须先经 `zoned_time` 转本地时间，否则跨时区会差一天。

**时间带依赖系统数据。** `zoned_time` 与 `locate_zone` 需要 tzdata；目标环境缺失时区数据库时，相关调用可能失败或回退，纯日历计算不受影响。

**C++20 支持分阶段。** GCC 11 提供日历与部分时间带，完整解析与流支持到 GCC 14；Clang 19 的 libc++ 时间带支持仍不完整；MSVC 19.29 起可用。跨工具链时应先用特性测试宏或最小示例验证。

**与 `std::tm` 互操作要换算。** `tm_year` 以 1900 为基准、`tm_mon` 从 0 开始；fmt 的适配代码展示了这两个偏移，自己转换时容易忘记。

## 库联动

- **`std::chrono::sys_days`**：`year_month_day` 与天数表示互转的桥梁，也是与 `time_point` 对接的入口。
- **`std::chrono::zoned_time`**：把系统时间映射到具体时区，是本地日期格式化的前提。
- **`std::format` / `std::formatter`**：C++20 格式化框架；库可以为 `year_month_day` 提供特化，直接把日期写进格式串。
- **`std::tm`**：传统 C 日期结构；与日历类型互操作时注意年份 1900 偏移和月份从 0 开始。
- **`std::chrono::weekday`**：从 `sys_days` 构造，用于星期查询与“下一个周一”一类的日期推算。

## 横向对比

| 方案                   | 类型安全 | 历法校验            | 时区支持          | 主要限制                           |
| ---------------------- | -------- | ------------------- | ----------------- | ---------------------------------- |
| `std::chrono` 日历     | 是       | `ok()`、`is_leap()` | 有（依赖 tzdata） | C++20 起；时间带支持分阶段         |
| `std::tm` + `time_t`   | 否       | 无                  | 依赖平台函数      | 字段偏移反直觉，闰年与天数要自己算 |
| 手写日期结构           | 是       | 自己实现            | 无                | 闰年、月份天数、星期都需自测       |
| Howard Hinnant date 库 | 是       | 有                  | 有                | 第三方依赖，标准日历的前身         |

日历类型由 P0355R7 进入 C++20，把日期从 `tm` 的整数字段变成可校验、可运算、可格式化的强类型。它不替代时区数据库，也不替调用方决定非法日期如何处理，但把闰年与月份天数这类容易出错的历法细节收进了标准库。
