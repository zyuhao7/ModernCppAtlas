---
id: cpp23.expected
title: std::expected
standard: C++23
kind: library
category: [错误处理, 类型安全]
difficulty: 3
status: final
tier: B
compilers: {gcc: "12", clang: "16", msvc: "19.33"}
tags: [错误传播, 单子操作, 无异常]
alternatives: [std::optional, std::variant, 错误码, 异常, tl::expected, absl::StatusOr]
proposal: P0323R12
last_verified: "2026-10-29"
---

## 解决什么问题

不用异常时，错误通常靠返回码或输出参数传递。返回码容易和正常值混在一起，输出参数要求调用方先声明再检查，漏检不会报错；`std::optional<T>` 能表达“没有值”，却丢掉了失败原因。`std::variant<T, E>` 虽然能同时表达两种状态，但两个分支都只是普通值，成功与失败的语义要靠约定。

`std::expected<T, E>` 把“成功值或错误”放进同一个返回类型：成功时持有 `T`，失败时持有 `E`。失败是类型的一部分，调用方可以从函数签名看出它可能失败，也可以在不启用异常的项目里逐层传播错误。

## 基础用法

<!-- compile -->

```cpp
#include <expected>
#include <iostream>
#include <string>
#include <string_view>

std::expected<int, std::string> parse_port(std::string_view text) {
  if (text.empty()) return std::unexpected(std::string{"empty port"});

  int value = 0;
  for (char c : text) {
    if (c < '0' || c > '9') {
      return std::unexpected(std::string{"port must be numeric"});
    }
    value = value * 10 + (c - '0');
  }
  if (value > 65535) return std::unexpected(std::string{"port out of range"});
  return value;
}

int main() {
  for (std::string_view text : {"8080", "abc", "70000"}) {
    auto result = parse_port(text);
    if (result) {
      std::cout << "port " << *result << '\n';
    } else {
      std::cout << "error: " << result.error() << '\n';
    }
  }
}
```

输出：

```text
port 8080
error: port must be numeric
error: port out of range
```

`result` 可以隐式转成 `bool`，为真表示持有值；`*result` 和 `result->` 访问成功值，`error()` 访问错误值。`value()` 也能取成功值，但在错误状态下会抛出 `std::bad_expected_access<E>`。只报告成功或失败、没有成功值的操作使用 `std::expected<void, E>`。

## 真实项目证据

| 项目               | 位置                                                                                                                                                                                              | 用法                                                             | 为什么典型                                                       |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------- | ---------------------------------------------------------------- |
| nghttp2/nghttp2    | [`src/shrpx_log.cc#L83-L91`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/shrpx_log.cc#L83-L91)                                                           | 把日志级别名称解析成整数，未知名称返回 `Error::INVALID_ARGUMENT` | 错误类型是项目内小枚举，成功值直接返回，调用方无需异常或输出参数 |
| hyprwm/Hyprland    | [`src/config/shared/parserUtils/ParserUtils.cpp#L16-L21`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/config/shared/parserUtils/ParserUtils.cpp#L16-L21) | 解析十六进制颜色分量，失败时把原始输入放进错误消息               | 解析层可以组合多个 `expected`，在解引用前统一检查失败            |
| stephenberry/glaze | [`include/glaze/net/http_client.hpp#L423-L428`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/net/http_client.hpp#L423-L428)                  | URL 解析失败时返回 `std::error_code`                             | 用标准错误码代替自定义错误类，失败与成功共用返回通道             |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **解引用前必须检查。** `operator*` 和 `operator->` 要求 `has_value()`，在错误状态下使用是未定义行为；`value()` 则抛出 `bad_expected_access<E>`。不要因为返回值不是指针就省略检查。
- **`expected<T&, E>` 不合法。** 成功值不能是引用；需要引用语义时用 `std::reference_wrapper<T>` 或指针，并自己保证生命周期。
- **`E` 不能是 `void` 或引用。** 只需要“有无值”而不需要原因时用 `std::optional<T>`；`std::expected<void, E>` 只表达成功或失败。
- **`value_or` 会按值返回。** 对昂贵类型它会复制或移动成功值；热路径上优先判断 `has_value()` 再取引用。
- **单子操作不会自动转换错误类型。** `and_then`、`or_else`、`transform`、`transform_error` 在失败时短路，但回调必须返回兼容的 `expected` 或 `unexpected`；跨错误类型的映射要显式写出来。
- **错误类型决定 ABI 与开销。** 用 `std::string` 或大对象作为 `E` 会把分配和对象布局带进接口；跨库边界优先用小枚举或 `std::error_code`。
- **它不消除异常。** 构造 `T`/`E`、调用 `value()` 仍可能抛出；`expected` 只是把可预期的失败放进返回值，不替代所有异常安全设计。

## 横向对比

| 方案                              | 携带失败原因 | 强制处理                   | 主要代价                                       |
| --------------------------------- | ------------ | -------------------------- | ---------------------------------------------- |
| `std::expected<T, E>`             | 是           | 类型可见，但仍可忽略返回值 | 错误类型进入公开 API；`value()` 可能抛异常     |
| `std::optional<T>`                | 否           | 需要检查 `has_value()`     | 丢失失败原因                                   |
| 返回错误码                        | 是（整数）   | 否，容易漏检               | 错误语义依赖约定，正常值与错误码混用           |
| `std::variant<T, E>`              | 是           | 需要 `visit` 或 `get`      | 成功与失败没有类型语义，样板代码多             |
| 异常                              | 是           | 否，可捕获也可穿透         | 部分项目禁用异常；二进制与性能约束需评估       |
| `tl::expected` / `absl::StatusOr` | 是           | 与 `expected` 类似         | C++20 及以前的替代；迁移时接口与错误类型要统一 |

`std::expected` 从 C++23 起可用；C++20 及以前的项目通常用 `tl::expected` 或 `absl::StatusOr` 过渡，迁移时重点检查错误类型和单子操作的回调签名。
