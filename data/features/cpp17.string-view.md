---
id: cpp17.string-view
title: std::string_view
standard: C++17
kind: library
category: [字符串, 视图]
difficulty: 3
status: final
tier: A
compilers: {gcc: "7", clang: "4", msvc: "19.11"}
tags: [非拥有视图, 零拷贝, 生命周期]
alternatives: [const std::string&, const char*, std::span<const char>]
proposal: P0254R2
last_verified: "2026-10-24"
claims:
  - id: c1
    statement: "用 is_same 排除 string_view 的隐式转换重载"
    evidence: [ev-json-string-view-exclusion-002]
  - id: c2
    statement: "验证 get<std::string_view>() 的取值语义"
    evidence: [ev-json-string-view-get-test-003]
  - id: c3
    statement: "用别名统一标准格式化后端的 string_view 类型"
    evidence: [ev-spdlog-string-view-alias-001]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

只读字符串参数写成 `const std::string&` 时，传入字符串字面量会构造临时 `std::string`（至少一次拷贝；短字面量通常走 SSO，不一定分配）；写成 `const char*` 时又丢失长度，遇到嵌入 `'\0'` 的文本会截断，还要反复调用 `strlen`。两个重载都写又会造成调用歧义。

`std::string_view` 是“指针 + 长度”的非拥有视图。它可以从字面量、`std::string`、字符数组零拷贝构造，按值传递成本低，并保留长度信息。它解决的是**只读字符串参数**问题；它不拥有字符，也不保证空终止。

## 基础用法

```cpp
#include <iostream>
#include <string>
#include <string_view>

std::string_view file_extension(std::string_view path) {
  auto slash = path.find_last_of('/');
  auto dot = path.find_last_of('.');
  if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) {
    return {};
  }
  return path.substr(dot + 1);
}

int main() {
  std::string path = "src/main.cpp";
  std::cout << file_extension(path) << '\n';
  std::cout << file_extension("README") << '\n';
}
```

完整示例见 [`examples/cpp17/string_view.cpp`](../../examples/cpp17/string_view.cpp)：

```text
[host] [example.com]
cpp

[alpha][beta][][gamma]
```

## 进阶用法

**不保证空终止。** `data()` 指向的缓冲区不一定以 `'\0'` 结尾；`substr`、`remove_prefix` 产生的视图更是如此。传给 `printf`、`strlen`、系统调用前必须复制成 `std::string`，或确认原缓冲区本身空终止且视图没有截断。

**长度是视图的一部分。** 字符串可以包含 `'\0'`，`size()` 仍然正确；但任何按 C 字符串处理的 API 都会在该字符处停止。二进制数据不要用 `string_view`，用 `span<const std::byte>`。

**生命周期是最常见的错误来源。** 视图不延长被指向对象的寿命。下面两种写法都会悬垂：

```cpp
std::string_view bad1() {
  std::string local = "temporary";
  return local;                 // local 销毁
}

std::string_view bad2() {
  return std::string("temporary");  // 临时 string 销毁
}
```

成员变量保存 `string_view` 时，必须确保被指向的字符串活得比对象久；`std::string` 的移动、重新分配、`clear` 都可能让旧视图失效。

**按值传递。** `string_view` 通常是两个机器字，复制廉价；参数写 `std::string_view` 而不是 `const std::string_view&`。需要修改或拥有数据时再接收 `std::string`。

**与 `const std::string&` 的重载冲突。** 同时提供 `f(std::string_view)` 与 `f(const std::string&)` 时，`std::string` 左值会优先匹配 `const std::string&`；字符串字面量对两者都是用户定义转换，可能产生歧义。通常只保留 `string_view` 版本，需要所有权时在函数内部复制。

**`remove_prefix` / `remove_suffix` 只移动视图。** 它们不修改原缓冲区，也不检查长度；传入超过 `size()` 的值是未定义行为。`substr` 在越界时会抛 `std::out_of_range`（位置参数），但 `remove_prefix` 不抛。

**C++20 增加前缀/后缀检查，C++23 增加 `contains`。** `starts_with`、`ends_with` 在 C++20 可用；`contains` 到 C++23 才加入。需要兼容 C++17 时仍用 `find`。

**哈希与容器。** `std::hash<std::string_view>` 可用，但把 `string_view` 作为 `unordered_map` 的键时，键只引用外部字符；外部字符串销毁后查找会读到悬垂内存。需要长期保存键时用 `std::string`。

## 真实项目证据

| 项目          | 位置                                                                                                                                                                  | 用法                                                           | 为什么典型                                           |
| ------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------- | ---------------------------------------------------- |
| gabime/spdlog | [`include/spdlog/common.h#L145-L156`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/common.h#L145-L156)               | 在标准格式化模式下把 `string_view_t` 定义为 `std::string_view` | 库内部用别名隔离两种格式化后端，调用方只依赖统一名称 |
| nlohmann/json | [`include/nlohmann/json.hpp#L1907-L1917`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/json.hpp#L1907-L1917)       | 用 `is_same<ValueType, std::string_view>` 排除隐式转换重载     | 防止 `string_view` 走隐式转换路径，避免悬垂与歧义    |
| nlohmann/json | [`tests/src/unit-conversions.cpp#L496-L502`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/tests/src/unit-conversions.cpp#L496-L502) | 验证 `get<std::string_view>()` 的取值语义                      | 测试锁定视图转换行为，补充生产路径                   |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。第 3 条来自测试，仅作补充。

## 陷阱与边界

- **返回局部字符串的视图是悬垂。** 函数返回 `string_view` 时，确保它指向调用方提供或静态存储的数据。
- **不保证空终止。** 不要直接传给 C API；需要时构造 `std::string`。
- **`remove_prefix` 不做边界检查。** 超过长度是未定义行为；先比较 `size()`。
- **`std::string` 的重新分配会让视图失效。** 保存视图期间不要修改原字符串的容量或内容。
- **`string_view` 不拥有数据。** 用它作为容器键或长期成员前，确认底层字符串的生命周期。
- **`data()` 可能为空。** 默认构造的视图 `data()` 可以是 `nullptr`；即使 `size() == 0`，也不要假定指针可解引用。
- **嵌入 `'\0'` 的文本会在 C API 处截断。** 视图本身保留长度，但下游不一定。
- **不要从 `std::string&&` 临时对象构造并保存视图。** 完整表达式结束后临时对象销毁，视图悬垂。

## 库联动

- **spdlog**：`string_view_t` 在标准格式化模式下是 `std::string_view`，在 fmt 模式下是 `fmt::string_view`；日志接口因此不用为两种后端写两套签名。代价是字符串必须活到日志调用完成。
- **nlohmann/json**：显式排除 `std::string_view` 的隐式转换重载，避免用户把视图直接转成 JSON 值后留下悬垂引用；需要复制时走 `std::string`。
- **fmt**：`fmt::string_view` 与 `std::string_view` 可互操作；格式化时把视图当只读输入，不复制。传给 `fmt::format` 的视图必须在调用期间有效。
- **与 `std::span<const char>` 的分工**：需要文本操作（查找、前缀、比较）用 `string_view`；只关心字节序列用 `span`。两者都不拥有数据。

## 横向对比

| 方案                    | 零拷贝                   | 携带长度 | 空终止保证 | 主要代价                      |
| ----------------------- | ------------------------ | -------- | ---------- | ----------------------------- |
| `std::string_view`      | 是                       | 是       | 否         | 生命周期由调用方保证          |
| `const std::string&`    | 否（字面量需构造临时串） | 是       | 是         | 可能分配与拷贝                |
| `const char*`           | 是                       | 否       | 是         | 需 `strlen`；嵌入 `'\0'` 截断 |
| `std::span<const char>` | 是                       | 是       | 否         | 无字符串操作接口              |
| `std::string`           | 否                       | 是       | 是         | 拥有数据，复制成本高          |

跨标准演进：C++17 引入 `std::string_view` 与 `sv` 字面量；C++20 增加 `starts_with`、`ends_with` 与更多 `constexpr` 支持；C++23 增加 `contains` 与范围构造。C++26 未改变核心语义。
