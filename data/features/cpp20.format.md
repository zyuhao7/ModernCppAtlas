---
id: cpp20.format
title: std::format
standard: C++20
kind: library
category: [格式化, 文本]
difficulty: 2
status: final
tier: B
compilers: {gcc: "13", clang: "17", msvc: "19.29"}
tags: [format, 格式化, 类型安全, 编译期检查]
alternatives: [printf, iostream, fmt, std::to_chars]
proposal: P0645R10
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "特性可用时用 std::format 生成 SQL 浮点参数，旧工具链回退到 stringstream"
    evidence: [ev-drogon-format-sql-double-001]
  - id: c2
    statement: "为自定义类型特化 std::formatter 并复用 std::format_to"
    evidence: [ev-spdlog-std-format-custom-type-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

`printf` 的格式串和实参类型分离，类型不匹配是未定义行为；`std::cout` 没有格式串，控制宽度、精度和进制要反复设置流状态。两者都缺少“格式串在编译期检查”的能力。

`std::format` 把 Python/fmt 风格的格式串带进标准库：普通字面量格式串在编译期检查，类型不匹配直接报错；结果返回 `std::string`，不依赖流状态；还提供 `format_to`、`format_to_n` 等写入迭代器的变体，避免临时字符串。

## 基础用法

<!-- compile -->

```cpp
#include <format>
#include <iostream>
#include <string>

int main() {
  std::string text = std::format("{:.2f} {} {}", 3.14159, "atlas", 42);
  std::cout << text << '\n';
}
```

`{:.2f}` 固定两位小数，`{}` 按类型选择默认格式。格式串是字面量时，编译器会检查占位符数量与实参类型；要复用运行期格式串，必须改用 `std::vformat`，并接受失去编译期检查的代价。

## 真实项目证据

| 项目   | 位置                                                                                                                                                               | 用法                                                 | 为什么典型                                                        |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ---------------------------------------------------- | ----------------------------------------------------------------- |
| Drogon | [`orm_lib/src/SqlBinder.cc#L286-L292`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/src/SqlBinder.cc#L286-L292) | 用 `std::format` 生成固定精度的 SQL 浮点参数         | 生产路径需要稳定的浮点文本；特性宏让旧工具链回退到 `stringstream` |
| spdlog | [`example/example.cpp#L283-L289`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/example/example.cpp#L283-L289)                    | 为自定义类型特化 `std::formatter` 并调用 `format_to` | 项目同时支持 fmt 与 `std::format`，切换后端不改变调用方写法       |

两条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。前者是库的生产路径，后者是随项目分发的示例代码。

## 陷阱与边界

**编译期检查只对常量格式串生效。** `std::format(runtime_string, value)` 不能通过 `std::format_string` 的检查；运行期格式串应使用 `std::vformat`，并自行承担格式错误的风险。

**`std::format` 返回新字符串。** 在热路径或大文本场景，优先用 `std::format_to` 写入已有缓冲区；`std::format_to_n` 可以限制输出长度，但不会自动截断到合法 UTF-8 边界。

**自定义类型需要 formatter。** 标准库不会通过 ADL 寻找 `format_as`；要为类型提供 `std::formatter<T>` 特化，实现 `parse` 与 `format`。这与 fmt 的扩展点相似，但不能直接复用 fmt 的 formatter。

**默认不跟随全局 locale。** 不要把 `std::format` 的输出与 `std::cout` 当前 locale 下的格式混为一谈；需要本地化数字或日期时，使用接受 locale 的接口或专门的本地化库。

**格式串语法不是 printf。** `%d`、`%s` 不适用；花括号要转义为 `{{`、`}}`。命名参数也不是 C++20 `std::format` 的一部分，不要照搬 fmt 的扩展语法。

**编译器支持差异要用特性宏判断。** 使用 `__cpp_lib_format` 检测 `<format>`；GCC 13、Clang 17 与 MSVC 19.29 才提供完整实现，旧标准库可能缺少部分格式化器。

**浮点输出依赖实现细节。** 标准规定了格式语法，但最短表示、舍入和特殊值文本在库实现之间可能有细微差异；跨平台测试要比较解析后的值，而不是逐字符比较。

## 横向对比

| 方案            | 类型安全 | 编译期检查   | 返回字符串 | 主要代价                               |
| --------------- | -------- | ------------ | ---------- | -------------------------------------- |
| `printf`        | 否       | 否           | 否         | 格式串与实参分离，错误是未定义行为     |
| `iostream`      | 是       | 否           | 否         | 流状态有粘性，格式化代码冗长           |
| `std::format`   | 是       | 是（字面量） | 是         | C++20 支持较新；自定义类型需 formatter |
| `fmt::format`   | 是       | 是           | 是         | 第三方依赖；扩展点更多，语法略有差异   |
| `std::to_chars` | 是       | —            | 否         | 只覆盖数值转换，不做通用文本拼接       |

跨标准演进：C++20 引入 `std::format`；C++23 增加 `std::print`、`std::println` 和更多格式化器；C++26 继续扩展格式语法与范围支持。`std::format` 与 fmt 共享设计血统，但标准版本的功能集合更保守。
