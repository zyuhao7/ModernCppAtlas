---
id: cpp23.expected
title: std::expected
standard: C++23
kind: library
category: [错误处理, 类型安全]
difficulty: 3
status: final
tier: A
compilers: {gcc: "12", clang: "16", msvc: "19.33"}
tags: [错误传播, 单子操作, 无异常]
alternatives: [std::optional, std::variant, 错误码, 异常, tl::expected, absl::StatusOr]
proposal: P0323R12
last_verified: "2026-11-18"
claims:
  - id: c1
    statement: "把用户输入解析成枚举值，失败时返回 std::unexpected，错误类型是项目内小枚举"
    evidence: [ev-nghttp2-expected-severity-001]
  - id: c2
    statement: "解析十六进制颜色分量，失败时把原始输入带进错误字符串"
    evidence: [ev-hyprland-expected-parse-hex-002]
  - id: c3
    statement: "URL 解析失败时返回 std::unexpected<std::error_code>，用标准错误码承载原因"
    evidence: [ev-glaze-expected-parse-url-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

不用异常时，错误通常靠返回码或输出参数传递。返回码容易和正常值混在一起，输出参数要求调用方先声明再检查，漏检不会报错；`std::optional<T>` 能表达“没有值”，却丢掉了失败原因。`std::variant<T, E>` 虽然能同时表达两种状态，但两个分支都只是普通值，成功与失败的语义要靠约定。

`std::expected<T, E>` 把“成功值或错误”放进同一个返回类型：成功时持有 `T`，失败时持有 `E`。失败是类型的一部分，调用方可以从函数签名看出它可能失败，也可以在不启用异常的项目里逐层传播错误。

## 基础用法

<!-- compile -->

```cpp
#include <charconv>
#include <expected>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

std::expected<int, std::string> parse_port(std::string_view text) {
  if (text.empty()) return std::unexpected(std::string{"empty port"});

  unsigned value = 0;
  const char* end = text.data() + text.size();
  const auto [next, error] = std::from_chars(text.data(), end, value);
  if (next != end) {
    return std::unexpected(std::string{"port must be numeric"});
  }
  if (error == std::errc::result_out_of_range || value > 65535u) {
    return std::unexpected(std::string{"port out of range"});
  }
  return static_cast<int>(value);
}

int main() {
  for (std::string_view text : {"8080", "abc", "70000", "99999999999999999999"}) {
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
error: port out of range
```

完整版本见 [`examples/cpp23/expected.cpp`](../../examples/cpp23/expected.cpp)，六个场景的输出为：

```text
[1] 8080 -> port 8080
[1] abc -> error: port must be numeric
[1] 70000 -> error: port out of range
[1] 99999999999999999999 -> error: port out of range
[2] value() threw, error = port must be numeric
[3] value_or(80) = 80, value_or(80) on success = 9000
[4] doubled(21) = 42, doubled(bad) has value = 0, error = port must be numeric
[5] default(abc) = 8080, default(1234) = 1234
[6] check(80) ok = 0, check(8080) ok = 1, error = privileged port
```

场景 1 用同一个返回类型表达三种结局：成功值、非数字、越界，调用方先判 `bool` 再取 `*result` 或 `error()`。场景 2 说明 `value()` 在失败时抛出 `std::bad_expected_access<E>`，异常携带原始错误对象。场景 3 对比 `value_or` 在成功与失败两种状态下的行为，它不区分失败原因。场景 4 的 `transform` 在失败时短路，回调不执行，错误原样保留。场景 5 的 `or_else` 把任意错误映射成兜底值 `8080`，成功路径不受影响。场景 6 使用 `std::expected<void, std::string>`：只有成功或失败，没有成功值，适合纯校验函数。

示例在标准库支持 `std::expected` 的单子操作时使用该类型。较旧的标准库用普通结构体重现相同输出。

`if (result)` 检查是否持有值，`*result` 和 `result->` 访问成功值，`error()` 访问错误值。`value()` 也能取成功值，但在错误状态下会抛出 `std::bad_expected_access<E>`。只报告成功或失败、没有成功值的操作使用 `std::expected<void, E>`。

## 进阶用法

**按接口边界选择错误类型。** nghttp2 用项目内枚举表示“名称无效”，错误值只是一个整数；glaze 直接用 `std::error_code`，让调用方沿用系统错误体系；Hyprland 把原始输入拼进 `std::string`，便于定位配置问题。错误类型会进入函数签名和 ABI，跨库接口优先用小枚举或 `std::error_code`，诊断信息放在日志层。

**用单子操作串联解析。** `and_then` 在前一步成功时继续调用返回 `expected` 的函数，失败则短路；`transform` 只映射成功值，`transform_error` 只映射错误值，`or_else` 可以替换失败路径。整条解析链保持“遇错即停、错误原样传递”，不必层层写 `if (!x) return ...`。

**`expected<void, E>` 表达纯校验。** 没有成功值、只关心成功或失败的函数用它作为返回类型，例如检查端口是否落在特权区间；调用方判 `bool` 即可，不需要为成功分支构造占位值。

**就地构造与移动。** `expected` 的构造函数把成功值或错误值直接构造在内部存储，返回 `std::unexpected` 时同样不产生额外临时对象；大对象作为 `E` 时要评估存储布局与移动成本。

**与 `value_or` 的取舍。** `value_or` 在失败时返回默认值，适合“缺失即可接受”的场景；需要保留错误原因或记录日志时，必须先判断 `has_value()` 再取 `error()`。

**注意编译器的设施门控。** 早期的 libstdc++ 在 `__cpp_concepts` 不足时会隐藏 `expected`；库代码应检测 `__cpp_lib_expected`，为旧工具链保留等价回退，而不是直接假定可用。

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

## 库联动

- **`std::unexpected`**：错误值的包装类型，构造函数把 `E` 直接放进 `expected` 的失败分支。
- **`std::error_code` / `std::errc`**：轻量错误类型，glaze 用它们让 `expected` 与系统错误体系对接，避免自定义异常类。
- **`std::optional`**：只表达“有无值”，不携带失败原因；需要诊断信息时用 `expected`。
- **`std::variant`**：通用的和类型，但成功与失败没有专门的语义和单子操作。
- **`std::exception_ptr`**：异常传播通道；`expected` 与它互补，分别承载可预期失败与异常。
- **`std::format`**：Hyprland 在构造错误信息时用它拼接原始输入，错误内容更易读。

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
