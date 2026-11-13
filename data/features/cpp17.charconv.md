---
id: cpp17.charconv
title: std::to_chars 与 std::from_chars
standard: C++17
kind: library
category: [字符串, 类型转换]
difficulty: 3
status: final
tier: A
compilers: {gcc: "8", clang: "7", msvc: "19.14"}
tags: [charconv, to_chars, from_chars, 数值解析, 格式化, 无分配]
alternatives: [std::to_string/std::sto*, strtol/strtod, std::format, ostringstream/istringstream]
proposal: P0067R5
last_verified: "2026-11-13"
claims:
  - id: c1
    statement: "命令行偏移解析用 std::from_chars，并用返回的 ptr 判断是否整串消费"
    evidence: [ev-hyprland-charconv-workspace-offset-001]
  - id: c2
    statement: "日志拼接器用 std::to_chars 以固定格式与精度把浮点写入已有缓冲区"
    evidence: [ev-nghttp2-charconv-log-float-002]
  - id: c3
    statement: "constexpr 解析函数用 std::from_chars 解析下标并借返回的 errc 判定成败"
    evidence: [ev-glaze-charconv-jmespath-int-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

数值与文本的互转，C++17 之前有两条路，都不理想。`std::to_string` / `std::sto*`（C++11）会分配临时 `std::string`，抛异常，且受全局区域设置影响——`to_string(1.5)` 在小数点上的行为依赖 locale。`sprintf` / `strtol` 一类 C 接口不分配，但要手写格式串、容易溢出、也没有区域无关保证。

热路径（日志、序列化、解析）需要的是：直接把数字写进调用方给的缓冲区，或直接从一段内存解析，不分配、不抛异常、不受 locale 影响、并且能报告“解析到哪、成功没有”。`<charconv>` 提供 `std::to_chars`（数字转文本）与 `std::from_chars`（文本转数字），满足全部这些要求。两者都是 `constexpr`。

## 基础用法

`std::to_chars(first, last, value)` 把 `value` 格式化到 `[first, last)`，返回 `{ptr, ec}`：`ptr` 指向写入末尾，`ec` 是错误码。`std::from_chars(first, last, value)` 反向解析，`ptr` 指向未消费的起点。头文件是 `<charconv>`。

<!-- compile -->

```cpp
#include <charconv>
#include <iostream>

int main() {
  char buf[32];
  auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), 12345);
  std::cout << std::string(buf, ptr) << '\n';
}
```

输出：

```text
12345
```

完整版本见 [`examples/cpp17/charconv.cpp`](../../examples/cpp17/charconv.cpp)，六个场景的输出为：

```text
[1] 12345
[2] ff
[3] parsed = 255, consumed = 1
[4] value = 42, stopped at 'a'
[5] error = true
[6] 3.14159265358979
```

场景 1 把整数写入栈缓冲区。场景 2 用基数 16 输出 `255` 得到 `ff`。场景 3 把 `ff` 按同一基数解析回 `255`，并确认整串被消费。场景 4 从 `42abc` 只解析出 `42`，返回的 `ptr` 停在 `a`。场景 5 对非法输入 `xyz`，错误码变成 `invalid_argument`，全程不抛异常。场景 6 把 `double` 转成文本，得到完整有效位。

## 进阶用法

**指针即进度。** `to_chars` 返回的 `ptr` 是写入末尾，可以接续拼下一段；`from_chars` 返回的 `ptr` 是首个未解析字符，既用来判断“整串是否都是数字”，也用来看停在哪个字符。Hyprland 解析工作区偏移时用 `END == value.data() + value.size()` 确认整串消费，避免把 `12x` 当成合法的 `12`。

**无异常的错误报告。** 两个函数都通过返回结构里的 `std::errc` 报错，不抛异常。这适合不能承受异常开销或作为 `noexcept` 的解析路径。glaze 的 constexpr 解析器直接检查 `result.ec == std::errc()` 来决定是否返回 `std::optional`。

**指定基与格式。** `to_chars` / `from_chars` 的整数重载接受 2–36 的基数，浮点重载接受 `std::chars_format`（`scientific`、`fixed`、`hex`、`general`）和可选精度。nghttp2 的日志用 `chars_format::fixed, 9` 固定九位小数，让输出稳定可预期。

**不分配、locale 无关。** 结果直接落在你给的缓冲区里，不构造临时字符串，也不读全局 locale。这是它相对 `to_string` / `sto*` 在热路径上的核心优势。

**constexpr。** 两个函数是 `constexpr`，能在编译期做数值与文本互转，或在常量表达式里的解析工具中使用。

**浮点支持要看标准库版本。** 早期实现里 `to_chars` / `from_chars` 的浮点重载可能缺失或不完整；整数重载先就绪。跨编译器时需确认浮点路径可用。

## 真实项目证据

| 项目     | 位置                                                                                                                                                                         | 用法                                                              | 为什么典型                                                   |
| -------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------- | ------------------------------------------------------------ |
| Hyprland | [`src/state/workspace/Resolver.cpp#L54-L66`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/state/workspace/Resolver.cpp#L54-L66)      | 用 `std::from_chars` 解析偏移，并用返回的 `ptr` 判断是否整串消费  | 命令行输入要求“整串是数字”，`ptr` 刚好用来验证无残留字符     |
| nghttp2  | [`src/shrpx_log.h#L175-L185`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/shrpx_log.h#L175-L185)                                    | 用 `std::to_chars` 以固定格式与精度把浮点写入日志缓冲区           | 日志热路径要不分配、不受 locale 影响，固定小数位保证输出稳定 |
| glaze    | [`include/glaze/json/jmespath.hpp#L297-L308`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/json/jmespath.hpp#L297-L308) | `constexpr` 解析函数用 `std::from_chars` 解析下标并借 `errc` 判定 | 解析器要 constexpr 且不抛异常，`errc` 提供了无异常的成功判据 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖整数解析、浮点格式化与 constexpr 解析三种用途。

## 陷阱与边界

**`from_chars` 不要求整串消费。** 它解析到不能继续为止就把控制权交回，`12x` 会成功解析出 `12`。要“整串必须是数字”必须自己检查返回的 `ptr` 是否等于末尾，这是最容易漏的一步。

**没有宽字符版本，也不处理符号以外的空白。** `from_chars` 不接受前导 `+` / 空白（部分实现接受 `-`），也不跳过空格。需要更宽松的解析时要自己预处理。

**返回的结构要解包。** 两个函数返回 `{ptr, ec}`，用结构化绑定接住是常规写法；忽略 `ec` 只在确定不会失败时才可以。

**浮点支持有版本门槛。** 整数重载早早可用，浮点重载在旧标准库里可能不完整甚至缺失，MSVC 尤其分化。用浮点路径前先确认目标标准库版本。

**与 locale 版转换的语义不同。** `strtod` 受 locale 影响、`from_chars` 不受；混用同一份数据可能得到不同结果。要可移植、可复现就统一走 `charconv`。

## 库联动

- **`std::string_view`**：解析对象常以 `string_view` 传入，`data()` 与 `size()` 直接给出解析区间。
- **`std::errc` / `<system_error>`**：`from_chars` 的错误码就是 `std::errc`，与错误处理设施一致。
- **`std::format`**（C++20）：格式化方向的后继，风格更接近 `printf`，但会分配；`to_chars` 仍是无分配场景的首选。
- **`std::to_string` / `std::sto*`**：C++11 的易用版本，适合一般业务代码；热路径与可复现场景改用 `charconv`。

## 横向对比

| 方案                           | 分配 | 抛异常 | locale 无关 | 主要限制                           |
| ------------------------------ | ---- | ------ | ----------- | ---------------------------------- |
| `std::to_chars` / `from_chars` | 否   | 否     | 是          | C++17 起；浮点支持依赖标准库版本   |
| `std::to_string` / `sto*`      | 是   | 是     | 否          | 受 locale 影响，精度固定           |
| `strtol` / `strtod`            | 否   | 否     | 否          | C 接口，受 locale 影响，易溢出错误 |
| `std::format`                  | 是   | 是     | 是          | C++20 起；保留分配                 |

`std::to_chars` 与 `std::from_chars` 由 P0067R5 进入 C++17，把“数字与文本互转”补齐到热路径可用的形态：不分配、不抛异常、locale 无关，还顺带成为 `constexpr`。它们不追求最易用的接口，而追求可预测的代价与结果，正适合日志、序列化与解析这类对性能与可复现性敏感的位置。
