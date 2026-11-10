---
id: cpp11.raw-string-literals
title: 原始字符串字面量（raw string literals）
standard: C++11
kind: language
category: [语法, 字符串]
difficulty: 1
status: final
tier: A
compilers: {gcc: "4.5", clang: "3.0", msvc: "16.0"}
tags: [原始字符串, 转义, 正则, JSON]
alternatives: [普通字符串加反斜杠转义, 拼接多行字面量, 从文件读取, 字符数组]
proposal: N2442
last_verified: "2026-11-10"
claims:
  - id: c1
    statement: "请求体 JSON 片段用原始字符串避免转义引号"
    evidence: [ev-vroom-raw-string-json-001]
  - id: c2
    statement: "连接串解析正则用原始字符串保留反斜杠"
    evidence: [ev-drogon-raw-string-regex-002]
  - id: c3
    statement: "RISC-V 扩展名解析用原始字符串编写正则"
    evidence: [ev-mold-raw-string-regex-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

普通字符串字面量里，反斜杠是转义字符：写一个 JSON 片段要转义双引号，写一条正则要双写每个反斜杠。内容越长，源码与真实文本的差异越大，读代码时必须在脑中反向解码。

```cpp
const char* json = "{\"name\":\"atlas\",\"size\":3}";
const char* regex = "(\\d+)\\.(\\d+)";
```

C++11 引入原始字符串字面量：`R"(...)"` 中的字符原样保留，只有 `)"` 会结束字符串。需要包含 `)"` 时可用带分隔符的形式 `R"tag(...)tag"`。标准提案为 N2442。

## 基础用法

用 `R"(` 开头、`)"` 结尾，中间内容不需要任何转义。

<!-- compile -->

```cpp
#include <iostream>
#include <string>

int main() {
  std::string pattern = R"(\d+\.\d+)";
  std::string json = R"({"name":"atlas","size":3})";
  std::cout << pattern << '\n';
  std::cout << json << '\n';
}
```

输出 `\d+\.\d+` 与 `{"name":"atlas","size":3}`。反斜杠与双引号都按字面保留。完整版本见 [`examples/cpp11/raw_string_literals.cpp`](../../examples/cpp11/raw_string_literals.cpp)。

## 进阶用法

**自定义分隔符。** `R"json({"text":")"})json"` 用 `json` 作为分隔符，内容里可以出现 `)"`；分隔符最多 16 个字符，不能含空格、括号、反斜杠。

**可以跨行。** 原始字符串直接包含换行，适合内嵌帮助文本、SQL、模板；源码中的换行就是字符串内容。

**与普通字符串拼接。** 相邻的普通字面量与原始字面量在翻译阶段拼接：`R"({"id":)" + std::to_string(id) + R"(})"`，VROOM 的请求构造常用这种写法。

**前缀组合。** 可以写 `u8R"(...)"`、`LR"(...)"`、`uR"(...)"`、`UR"(...)"`，分别对应 UTF-8、宽字符与 UTF-16/32。

**结尾换行属于内容。** 如果 `)"` 单独占一行，字符串末尾会带一个换行符；需要精确控制输出时把结束定界符紧跟在内容后。

**不是所有场景都更好。** 短字符串用普通转义更紧凑；原始字符串适合含大量引号、反斜杠或多行的文本。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                 | 用法                              | 为什么典型                         |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------- | ---------------------------------- |
| VROOM  | [`src/routing/ors_wrapper.cpp#L14-L22`](https://github.com/VROOM-Project/vroom/blob/43dd7d0b8b560431eb555bf335cf4797eb7343c4/src/routing/ors_wrapper.cpp#L14-L22)    | JSON 请求片段用原始字符串         | 报文中的双引号在源码里原样保留     |
| drogon | [`orm_lib/src/DbConnection.cc#L21-L31`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/src/DbConnection.cc#L21-L31) | 连接串解析正则用原始字符串        | 反斜杠与正则语义一一对应，无需双写 |
| mold   | [`src/arch-riscv.cc#L1076-L1082`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/arch-riscv.cc#L1076-L1082)                        | RISC-V 扩展名解析用原始字符串正则 | 分组与 `\d` 转义直接可读           |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖 JSON 报文与正则表达式两类典型内容。

## 陷阱与边界

**只有 `)"` 会结束字符串。** 单独的 `)` 或 `"` 都没问题；需要内容包含 `)"` 时必须换分隔符，不能靠转义。

**转义序列全部失效。** `\n`、`\t`、`\\` 在原始字符串里都是两个字面字符；需要换行时直接敲回车。

**分隔符规则有限。** 分隔符最多 16 个字符，不能包含空格、左括号、右括号、反斜杠和控制字符；`R"("` 不是合法前缀。

**源码换行影响内容。** 跨行原始字符串会把缩进与换行原样带入；生成 JSON 时容易多出空白，需要刻意控制排版。

**编译器对 `u8` 前缀的历史差异。** C++11 的 `u8R` 产生 `const char[]`，C++20 起类型变为 `const char8_t[]`；跨标准代码要注意类型变化。

**MSVC 支持较早但仍需确认。** VS2010（16.0）已支持原始字符串，但早期版本对自定义分隔符的边界行为有缺陷，老工具链需实测。

## 库联动

- **`std::string`**：原始字面量可直接构造 `std::string`，包含的换行与引号不经过转义。
- **`std::regex`**：正则模式用原始字符串编写，反斜杠与分组语法保持原样。
- **JSON 库**：`nlohmann/json`、glaze 等把原始字符串作为常量 JSON 的常见来源。
- **`std::format` / `fmt`**：格式化字符串本身含花括号时，原始字符串可避免额外转义。
- **SQL 与脚本内嵌**：多行 SQL、shell 片段直接以原始字符串嵌入，不需要拼接。
- **VROOM / drogon / mold**：分别展示 JSON 报文与正则表达式两类生产用法。

## 横向对比

| 方案             | 可读性         | 多行支持    | 主要限制                   |
| ---------------- | -------------- | ----------- | -------------------------- |
| 原始字符串字面量 | 高，内容即源码 | 直接支持    | 需要处理结束定界符与分隔符 |
| 普通字符串转义   | 低，需反向解码 | 需显式 `\n` | 引号与反斜杠逐处转义       |
| 相邻字面量拼接   | 中             | 需手动拼接  | 拼接处容易漏空格或换行     |
| 从文件读取       | 高             | 天然支持    | 引入运行时 I/O 与文件依赖  |

跨标准演进：C++11 通过 N2442 统一原始字符串与 Unicode 字面量；C++14 增加 `s` 后缀等字面量增强；C++20 把 `u8` 字面量类型改为 `char8_t`，原始字符串前缀的语义随之调整，但 `R"(...)"` 语法本身未变。
