---
id: cpp20.source-location
title: std::source_location
standard: C++20
kind: library
category: [诊断, 日志]
difficulty: 2
status: final
tier: B
compilers: {gcc: "11", clang: "16", msvc: "19.29"}
tags: [日志, 调试, 诊断, 调用点, 宏替代]
alternatives: [__FILE__ 与 __LINE__, std::stacktrace, 日志库宏]
proposal: P1208R6
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "为 std::source_location 提供格式化器，直接输出文件、行号、列号与函数名"
    evidence: [ev-fmt-source-location-formatter-001]
  - id: c2
    statement: "在标准类型可用时把兼容类型别名到 std::source_location"
    evidence: [ev-abseil-source-location-alias-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

诊断信息需要知道“这次调用来自哪里”。传统写法是让调用方传 `__FILE__`、`__LINE__`，或者用宏把位置拼进函数参数。宏会污染调用点，参数列表也会变长；日志库往往还要维护一整套 `LOG_INFO(...)` 宏来捕获位置。

`std::source_location` 把文件名、行号、列号和函数名打包成一个轻量值。把它写成函数的默认实参，调用方不传任何东西，位置就自动记录在调用点。

## 基础用法

<!-- compile -->

```cpp
#include <iostream>
#include <source_location>

void log(const std::source_location& location =
             std::source_location::current()) {
  std::cout << location.file_name() << ':' << location.line() << ' '
            << location.function_name() << '\n';
}

int main() {
  log();
}
```

`std::source_location::current()` 在默认实参中求值时，记录的是调用 `log()` 的位置，而不是 `log` 函数体内部的位置。`file_name()`、`line()`、`column()`、`function_name()` 都是只读查询，不需要额外分配。

## 真实项目证据

| 项目       | 位置                                                                                                                                                              | 用法                                             | 为什么典型                                                     |
| ---------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ | -------------------------------------------------------------- |
| fmt        | [`include/fmt/std.h#L480-L494`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/std.h#L480-L494)                          | 为 `std::source_location` 特化格式化器           | 日志库需要在格式串里直接输出调用位置，格式化器让调用点保持简洁 |
| abseil-cpp | [`absl/types/source_location.h#L57-L65`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/types/source_location.h#L57-L65) | 标准类型可用时把 `absl::SourceLocation` 别名过去 | 兼容层避免维护两套位置类型，接口可以逐步迁移到标准类型         |

两条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。两者都用特性测试宏保护标准类型，避免旧工具链直接包含 `<source_location>`。

## 陷阱与边界

**默认实参才记录调用点。** 在函数体里直接调用 `std::source_location::current()`，得到的是那一行的位置；要捕获调用方位置，必须把它放在默认实参或调用点的立即求值位置。

**位置字符串不是稳定接口。** `file_name()` 可能是相对路径或绝对路径，`column()` 的实现和起始值由实现定义，`function_name()` 可能是空串或带签名的文本。不要把这些字符串用于逻辑判断或持久化。

**它描述源码位置，不是调用栈。** `source_location` 只有一层调用点；需要完整调用链时用 `std::stacktrace`（C++23）或平台回溯设施。

**没有统一的流输出。** `std::source_location` 不提供 `operator<<`；要直接打印，需要自定义格式化器、`std::format` 的 formatter，或手动拼接四个字段。

**拷贝很便宜，但不要保存悬垂引用。** `source_location` 是值类型，按值或 `const&` 传递都可以；保存 `file_name()` 返回的 `const char*` 时，要确认它指向的字符串生命周期覆盖使用范围。

**特性支持要检测。** 使用 `__cpp_lib_source_location` 判断 `<source_location>` 是否可用；Clang 15 只有部分支持，16 才完整，不能只看 `__cplusplus` 的值。

## 横向对比

| 方案                       | 调用方负担 | 类型安全 | 捕获列号/函数名 | 主要代价                            |
| -------------------------- | ---------- | -------- | --------------- | ----------------------------------- |
| `__FILE__` / `__LINE__` 宏 | 需要展开宏 | 否       | 否              | 宏污染、字符串拼接、难以封装        |
| `std::source_location`     | 无         | 是       | 是              | 需要 C++20；字符串内容实现定义      |
| 日志库的 `LOG_*` 宏        | 无         | 部分     | 取决于库        | 绑定日志框架，跨库传递位置需要适配  |
| `std::stacktrace`          | 无         | 是       | 是（调用栈）    | C++23；捕获成本更高，符号化依赖平台 |
| 手动传入文件、行号参数     | 高         | 是       | 可选            | 每个调用点重复传参，容易传错        |

跨标准演进：C++20 引入 `std::source_location`；C++23 增加 `std::stacktrace`，两者互补：前者标记单个源码位置，后者记录调用链。
