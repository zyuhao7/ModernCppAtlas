---
id: cpp11.string-conversions
title: std::to_string 与 std::sto* 数值转换
standard: C++11
kind: library
category: [字符串, 类型转换]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.5", clang: "3.0", msvc: "18.0"}
tags: [to_string, stoi, stod, 数值转换, 异常]
alternatives: [ostringstream/istringstream, std::from_chars/std::to_chars, strtol/strtod, std::format, sprintf]
proposal: N2408
last_verified: "2026-11-12"
claims:
  - id: c1
    statement: "诊断信息用 std::to_string 把行号拼进字符串标签"
    evidence: [ev-mold-string-conversions-line-label-001]
  - id: c2
    statement: "解析 IPC 参数时用 std::stoi 并捕获转换异常"
    evidence: [ev-hyprland-string-conversions-ipc-int-002]
  - id: c3
    statement: "从存储元数据字符串解析块号用 std::stoll"
    evidence: [ev-duckdb-string-conversions-checkpoint-stoll-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++03 时代，数字与字符串互转要么走 `std::ostringstream` / `std::istringstream`，要么退回 `sprintf` 和 `strtol`。前者要构造流对象、设置格式、再读取结果，样板代码多；后者使用 C 风格缓冲区，长度和错误检查都要手写，稍不注意就引入越界或截断。

C++11 通过 N2408 补上了两组直接接口：`std::to_string` 把整数和浮点数转成 `std::string`，`std::stoi`、`std::stol`、`std::stoll`、`std::stof`、`std::stod`、`std::stold` 把字符串转成对应数值。转换函数用异常报告解析失败，并可通过输出参数告诉调用方消费了多少字符，比流式解析更直接。

## 基础用法

`std::to_string` 对每种算术类型有重载，返回十进制文本。`std::sto*` 系列跳过前导空白，解析尽可能长的合法前缀，返回数值；如果传入 `pos`，它会被写成第一个未消费字符的下标。

<!-- compile -->

```cpp
#include <iostream>
#include <string>

int main() {
  std::cout << std::to_string(42) << ' ' << std::to_string(3.5) << '\n';

  std::size_t pos = 0;
  int value = std::stoi("42abc", &pos);
  std::cout << value << ' ' << pos << '\n';
}
```

输出：

```text
42 3.500000
42 2
```

完整版本见 [`examples/cpp11/string_conversions.cpp`](../../examples/cpp11/string_conversions.cpp)，六个场景的输出为：

```text
[1] 42 -7 9000000000 3.500000
[2] parsed = 42, consumed = 2
[3] hex = 255
[4] invalid_argument caught
[5] out_of_range caught
[6] ratio = 3.14159, used = 7
```

场景 1 展示整数与浮点的默认格式：整数精确输出，`double` 用固定六位小数。场景 2 说明 `stoi` 只解析合法前缀，`pos` 指出 “42abc” 里只消费了前两个字符。场景 3 用第三个参数指定十六进制。场景 4 对完全非数字的输入捕获 `std::invalid_argument`。场景 5 用超出 `int` 范围的字符串触发 `std::out_of_range`。场景 6 的 `stod` 解析带小数点的前缀，并把尾部文本留在字符串里。

整数转换默认按十进制解析；需要二进制、八进制或十六进制时，把进制作为最后一个参数传入。`std::sto*` 的 `pos` 参数可以为 `nullptr`，表示不关心消费位置。

## 进阶用法

**区分“前缀解析”和“完整解析”。** `stoi("42abc")` 返回 42 而不报错，这对解析带单位的输入很方便，但对必须完整的配置值很危险。要求整个字符串都被消费时，解析后检查 `pos == text.size()`；否则应拒绝输入。

**两类异常。** 没有任何合法前缀时抛 `std::invalid_argument`；前缀合法但结果超出目标类型范围时抛 `std::out_of_range`。两者都继承自 `std::logic_error` 体系，可以分别捕获，也可以统一按 `std::exception` 处理。

**浮点转换有精度与格式差异。** `std::to_string(double)` 使用固定六位小数，既可能丢失有效数字，也无法表达科学计数法；`std::stod` 能解析科学计数法，但结果受 `strtod` 语义影响。需要可逆或高精度文本时，改用 `std::to_chars` / `std::from_chars`（C++17）或 `std::format`（C++20）。

**宽字符版本。** 每个函数都有 `std::wstring` 重载，例如 `std::to_wstring`、`std::stoi(const std::wstring&, ...)`。它们与窄字符版本行为一致，只是字符类型不同。

**异常不是唯一选择。** C++17 的 `std::from_chars` 不分配、不抛异常，用返回的 `ptr` 与 `ec` 报告结果，适合解析热路径；代价是浮点支持在旧标准库中不完整，且没有宽字符版本。

## 真实项目证据

| 项目     | 位置                                                                                                                                                                          | 用法                                          | 为什么典型                                 |
| -------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------- | ------------------------------------------ |
| mold     | [`src/linker-script.cc#L40-L40`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/linker-script.cc#L40-L40)                                   | 把行号转成文本后拼进错误标签                  | 诊断输出只需要一次简单转换，流对象反而累赘 |
| Hyprland | [`src/ipc/s1/Commands.cpp#L1998-L2003`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/ipc/s1/Commands.cpp#L1998-L2003)                 | 解析字体大小参数，用 `try/catch` 兜住转换失败 | IPC 输入来自外部，必须先校验再进入渲染路径 |
| duckdb   | [`src/storage/checkpoint_manager.cpp#L272-L274`](https://github.com/duckdb/duckdb/blob/069cc9f9b5be802405797faecc284961b07c70ef/src/storage/checkpoint_manager.cpp#L272-L274) | 把元数据里逗号分隔的块 ID 转成 64 位整数      | 与内部块号类型一致，省去手写解析循环       |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，分别对应诊断拼接、外部输入校验和内部元数据解析。

## 陷阱与边界

**尾部垃圾不会报错。** `stoi("42abc")` 成功返回 42；只有 `pos` 能揭示剩余内容。配置、协议字段等要求完整的输入必须显式检查消费位置。

**溢出抛异常而不是饱和。** 超出 `int` 的结果触发 `std::out_of_range`，不会回绕。选择目标类型时要考虑输入范围，或者直接使用更宽的 `stoll` / `stoull`。

**`to_string` 的浮点格式固定且不可配置。** 它等价于 `sprintf("%f")` 风格输出，固定六位小数，既不保证最短表示，也不保证往返精确。对 `1e-10` 这类值会得到 `0.000000`。

**受当前 C 区域设置影响。** 底层使用 `strtod`、`sprintf` 一类函数，小数分隔符可能随 `setlocale` 变化。协议和持久化格式应使用与区域无关的转换。

**转换会分配内存。** `to_string` 返回 `std::string`，`sto*` 在解析时也可能构造临时对象；热路径上优先考虑 `std::to_chars` / `std::from_chars`。

**异常边界要明确。** 在 `noexcept` 函数或异常关闭的构建里调用 `sto*`，失败会直接终止程序。外部输入解析应在外层捕获，或改用不抛异常的接口。

## 库联动

- **`std::string` / `std::wstring`**：两组转换都围绕字符串类型设计，窄字符与宽字符版本一一对应。
- **`std::from_chars` / `std::to_chars`**（C++17）：不分配、不抛异常、与区域设置无关，是高性能解析与格式化的后继方案。
- **`std::format`**（C++20）：需要可配置格式、精度和宽度时，用格式化库替代 `to_string`。
- **`std::ostringstream` / `std::istringstream`**：需要流式格式控制或自定义 `operator<<` / `operator>>` 时仍然适用。
- **诊断与日志**：mold 一类工具直接把行号、偏移等整数拼进错误消息，`to_string` 是最轻的写法。

## 横向对比

| 方案                                   | 解析失败报告     | 分配 | 区域设置敏感 | 主要限制                         |
| -------------------------------------- | ---------------- | ---- | ------------ | -------------------------------- |
| `std::to_string` / `std::sto*`         | 异常             | 是   | 是           | 浮点格式固定；尾部垃圾不报错     |
| `std::from_chars` / `to_chars`         | 返回 `errc`      | 否   | 否           | C++17 起；浮点支持依赖标准库版本 |
| `std::ostringstream` / `istringstream` | 流状态           | 是   | 是           | 样板代码多，性能开销大           |
| `sprintf` / `strtol`                   | 返回值与 `errno` | 否   | 是           | 缓冲区长度与错误检查需手动管理   |
| `std::format`                          | 异常             | 是   | 否           | C++20 起；依赖格式化库实现       |

`std::to_string` 与 `std::sto*` 由 N2408 进入 C++11，填补了标准库缺少直接数值转换的空白。它们适合一般业务代码和诊断输出；对性能、区域无关性或往返精确性有要求时，应转向 `from_chars` / `to_chars` 或 `std::format`。
