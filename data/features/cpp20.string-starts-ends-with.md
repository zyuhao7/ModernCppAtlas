---
id: cpp20.string-starts-ends-with
title: string::starts_with / ends_with
standard: C++20
kind: library
category: [字符串, 性能]
difficulty: 1
status: final
tier: A
compilers: {gcc: "9", clang: "6", msvc: "19.21"}
tags: [starts_with, ends_with, 前后缀, 字符串匹配, string_view]
alternatives: [compare 前缀比较, substr 比较, find 等于 0, mismatch, strncmp]
proposal: P0457R2
last_verified: "2026-11-18"
claims:
  - id: c1
    statement: "链接器用 string_view::starts_with 判断输入是否以 ELF 魔数开头"
    evidence: [ev-mold-starts-with-magic-001]
  - id: c2
    statement: "命令行工具同时用 starts_with 与 ends_with 识别 Lua REPL 的不完整行错误"
    evidence: [ev-hyprland-starts-with-reply-002]
  - id: c3
    statement: "路由后端用 starts_with 检查运行期错误消息是否以局部 std::string 保存的固定前缀开头"
    evidence: [ev-vroom-starts-with-error-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

判断一个字符串是否以指定前缀开头或后缀结尾，在 C++20 之前要绕几道弯。`compare(0, prefix.size(), prefix) == 0` 要求调用方自己算长度，长度写错不会报错；用 `substr` 比较会分配临时字符串；`find(prefix) == 0` 虽然可用，但和后缀检查不对称。C 风格代码则依赖 `strncmp`，只能处理以空字符结尾的字符数组。

P0457R2 给 `std::basic_string` 和 `std::basic_string_view` 增加 `starts_with` 与 `ends_with`。两者都接受 `string_view`、单个字符和 C 字符串重载，直接表达“前缀/后缀”意图，比较过程不分配内存。C++20 起可用，特性测试宏为 `__cpp_lib_starts_ends_with >= 201711L`。「字符串与范围小工具」页把它和 `erase_if`、`ssize` 放在一起讲，那一页的编译器版本按整组设施中最晚可用的一项计算，所以比本页高。

## 基础用法

前缀与后缀检查都是成员函数调用，返回 `bool`。

<!-- compile -->

```cpp
#include <iostream>
#include <string>

int main() {
  const std::string path = "/usr/local/bin/tool";
  std::cout << path.starts_with('/') << ' '
            << path.ends_with("tool") << '\n';
}
```

输出 `1 1`。完整版本见 [`examples/cpp20/string_starts_ends_with.cpp`](../../examples/cpp20/string_starts_ends_with.cpp)，六个场景的输出为：

```text
[1] absolute = 1
[2] tool = 1, l = 1
[3] gz = 1, bz2 = 0
[4] archive = 1, tar = 0
[5] empty = 1, 1
[6] case = 0
```

场景 1 用字符重载检查绝对路径。场景 2 对同一个字符串分别做字符串后缀与字符后缀检查。场景 3 在 `std::string_view` 上做后缀匹配。场景 4 用前缀检查文件名开头。场景 5 说明空前缀和空后缀恒为真。场景 6 显示比较区分大小写。

每个函数有三个重载：接受 `basic_string_view`、接受单个字符、接受以空字符结尾的 C 字符串。`basic_string_view` 版本按长度比较，不会因为实参中嵌入空字符而截断。

## 进阶用法

**用字面量后缀传前缀。** 把 `using namespace std::literals` 打开后，`"error: "sv` 的类型是 `string_view`，直接传给 `starts_with`，不构造 `std::string`。这对高调用频率的分类代码有意义：前缀检查本身是 O(前缀长度)，不应该附带一次分配。

**把前缀写成常量。** 路由层或错误处理层常把错误前缀集中定义一次，再用 `starts_with` 匹配原始消息。前缀可以是 `std::string`、`string_view` 或字面量，重载会自动选择。

**前后缀一起用。** 解析带包裹标记的文本时，`starts_with(open) && ends_with(close)` 比两次 `substr` 或 `find` 更贴近语义，也避免构造中间字符串。

**替代 `compare` 与 `find`。** `s.compare(0, n, prefix) == 0` 与 `s.starts_with(prefix)` 等价，后者不需要手写 `n`；`s.find(prefix) == 0` 在长字符串上可能继续搜索后续位置，而 `starts_with` 只比较开头。

**字符重载用于分隔符。** 检查路径是否以 `/` 开头、选项是否以 `-` 结尾时，传字符比构造单字符字符串更直接。

## 真实项目证据

| 项目                | 位置                                                                                                                                                                              | 用法                                               | 为什么典型                                            |
| ------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------- | ----------------------------------------------------- |
| rui314/mold         | [`src/filetype.cc#L77-L77`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/filetype.cc#L77-L77)                                                 | 用 `starts_with` 判断 ELF 魔数                     | 文件类型识别是热路径，前缀检查直接、无额外分配        |
| hyprwm/Hyprland     | [`hyprctl/src/main.cpp#L280-L280`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/hyprctl/src/main.cpp#L280-L280)                               | 同时用 `starts_with` 与 `ends_with` 识别 REPL 错误 | 前后缀对称地表达“某种包裹格式”，条件可读性高          |
| VROOM-Project/vroom | [`src/routing/osrm_routed_wrapper.cpp#L72-L74`](https://github.com/VROOM-Project/vroom/blob/43dd7d0b8b560431eb555bf335cf4797eb7343c4/src/routing/osrm_routed_wrapper.cpp#L72-L74) | 用 `starts_with` 匹配错误消息前缀                  | 前缀存于局部 `std::string`，经 `string_view` 重载比较 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

**比较区分大小写。** `starts_with` 和 `ends_with` 不做大小写折叠；需要忽略大小写时要自己转换或使用其他设施。

**空前缀、空后缀恒为真。** `s.starts_with("")` 与 `s.ends_with("")` 对任意字符串都返回 `true`，包括空字符串；用空字符串表示“没有前缀”时不要依赖这个结果做否定判断。

**没有起始位置参数。** 只检查整个字符串的开头或结尾。要检查从某个偏移开始的前缀，需要用 `std::string_view(s).substr(pos).starts_with(prefix)` 或先切片。

**`string_view` 版本不会在空字符处截断。** 它按长度比较，实参可以包含嵌入的 `'\0'`；这与接受 C 字符串的重载不同，后者在第一个空字符处结束。

**这些成员只属于字符串类型。** `basic_string` 和 `basic_string_view` 提供前缀/后缀检查，`std::vector<char>` 等任意字符容器没有同名成员。

**返回值是 `bool`，不给出位置。** 需要知道第一个不匹配位置或提取被匹配部分时，仍要用 `compare`、`mismatch` 或手动切片。

**支持时间。** GCC 9、Clang 6、MSVC 19.21 起提供；特性测试宏为 `__cpp_lib_starts_ends_with >= 201711L`。

## 库联动

- **`std::string_view`**：前缀参数与成员本身都围绕视图设计，传视图重载不构造临时 `std::string`。
- **`std::string::compare`**：通用比较接口，能表达前缀比较但需要显式长度。
- **`std::mismatch`**：算法层的逐元素比较，需要同时提供两端区间，适合需要不匹配位置的场景。
- **`std::filesystem::path`**：路径有自己的 `extension()`、`filename()` 等接口，判断扩展名通常比字符串后缀更语义化。
- **`std::format`**：格式化消息时可用前缀检查决定日志级别，两者常出现在同一条错误处理路径。

## 横向对比

| 方案                        | 处理前缀 | 处理后缀 | 需要临时字符串 | 主要限制                            |
| --------------------------- | -------- | -------- | -------------- | ----------------------------------- |
| `starts_with` / `ends_with` | 是       | 是       | 否             | C++20 起可用；区分大小写            |
| `compare(0, n, prefix)`     | 是       | 否       | 否             | 需要手写长度，后缀需自己算偏移      |
| `substr(...) == prefix`     | 是       | 是       | 是             | 产生临时字符串，长字符串上有分配    |
| `find(prefix) == 0`         | 是       | 否       | 否             | 只解决前缀，语义不如专用函数直接    |
| `strncmp` / `memcmp`        | 是       | 否       | 否             | 面向 C 字符串或裸缓冲区，需自算长度 |

P0457R2 把前后缀判断从“比较长度与偏移”的算术问题，变成一次意图明确的成员调用。它不改变复杂度，但去掉了手写长度和临时字符串这两个常见错误来源。
