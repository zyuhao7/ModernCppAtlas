---
id: cpp17.optional
title: std::optional
standard: C++17
kind: library
category: [错误处理, 值语义]
difficulty: 3
status: final
tier: A
compilers: {gcc: "7", clang: "5", msvc: "19.11"}
tags: [可选值, 值语义, 空状态]
alternatives: [指针, std::unique_ptr, std::variant, std::expected]
proposal: P0220R1
last_verified: "2026-10-24"
claims:
  - id: c1
    statement: "为空状态输出 none，有值时输出 optional(value)"
    evidence: [ev-fmt-optional-formatter-001]
  - id: c2
    statement: "JSON null 映射为 nullopt，其他值 emplace"
    evidence: [ev-json-optional-from-json-002]
  - id: c3
    statement: "有值写值，空状态写 null"
    evidence: [ev-json-optional-to-json-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

“可能没有值”在 C++17 之前通常用三种方式表达：返回指针并用 `nullptr` 表示失败；返回 `bool` 并通过输出参数写结果；或者抛异常。指针引入所有权与生命周期问题，输出参数让调用点冗长，异常不适合“正常但为空”的情况（例如查找未命中）。

`std::optional<T>` 把“有值/无值”编码进类型。它按值存储 `T`，不分配堆内存；调用方必须显式检查状态，或在确定有值时解引用。它表达的是**值可能缺失**，不是**操作失败的原因**。

## 基础用法

<!-- compile -->

```cpp
#include <charconv>
#include <iostream>
#include <optional>
#include <string_view>

std::optional<int> parse_int(std::string_view text) {
  int value = 0;
  auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || ptr != text.data() + text.size()) {
    return std::nullopt;
  }
  return value;
}

int main() {
  if (auto parsed = parse_int("42")) {
    std::cout << *parsed << '\n';
  }
  std::cout << parse_int("nope").value_or(-1) << '\n';
}
```

完整示例见 [`examples/cpp17/optional.cpp`](../../examples/cpp17/optional.cpp)：

```text
127.0.0.1
8080
-1
30
present: 1, enabled: 0
atlas
none
```

示例把 `optional` 用在配置读取上：查找可能缺失的键、解析可能失败的数字、用 `value_or` 提供默认值，并单独演示 `optional<bool>` 的“有值”与“值为真”不是一回事。

## 进阶用法

**解引用不检查。** `*opt` 与 `opt->member` 在空状态下是未定义行为；`value()` 会抛 `std::bad_optional_access`。性能敏感路径用 `has_value()` 或 `if (opt)` 分支，异常路径用 `value()`。

**`if (opt)` 判断的是“是否有值”。** 对 `std::optional<bool>`，`if (opt)` 为真只说明里面装了一个 `bool`，不代表该 `bool` 为 `true`；要读值写 `if (opt && *opt)`。这是最常见的误用。

**比较语义是“空小于有值”。** 两个 `optional` 比较时，先比状态，再比值；`nullopt < optional{0}`。与 `T` 比较时，空状态不等于任何值。不要用比较来判断“值是否满足业务条件”，先检查状态。

**`optional<T&>` 不合法。** 标准明确禁止可选引用；需要“可能没有的引用”时用 `T*`（可空、不拥有）或 `std::reference_wrapper<T>` 的 `optional`。两者语义不同：指针可为空，`reference_wrapper` 一旦构造就必须绑定有效对象。

**`emplace` 会先销毁旧值。** 若新值的构造函数抛异常，`optional` 处于空状态，旧值不会恢复。需要强异常保证时，先构造临时对象再赋值/`emplace`。

**不分配堆内存，但有空间开销。** `optional<T>` 通常包含一个 `bool` 标志与对齐后的 `T` 存储；`sizeof(std::optional<char>)` 往往大于 1。对超大数组，用 `std::vector<T>` 加单独的位图可能更省空间。

**C++23 才有单子操作。** `and_then`、`transform`、`or_else` 在 C++23 加入；C++17 需要手写 `if` 链。它们不会自动处理异常，只处理空状态。

**与 `std::expected` 的分工。** `optional` 只表达“没有值”，不携带失败原因；需要错误码或错误消息时用 `expected<T, E>`（C++23）。不要为了塞错误信息把 `optional` 的 `T` 变成 `pair<bool, Error>`。

## 真实项目证据

| 项目          | 位置                                                                                                                                                                                                    | 用法                                              | 为什么典型                                     |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------- | ---------------------------------------------- |
| fmtlib/fmt    | [`include/fmt/std.h#L416-L427`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/std.h#L416-L427)                                                                | 为空状态输出 `none`，有值时输出 `optional(value)` | 把可选值的两种状态显式呈现，避免日志里出现歧义 |
| nlohmann/json | [`include/nlohmann/detail/conversions/from_json.hpp#L49-L62`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/detail/conversions/from_json.hpp#L49-L62) | JSON `null` 映射为 `nullopt`，其他值 `emplace`    | 反序列化时把“字段缺失/为空”统一成类型状态      |
| nlohmann/json | [`include/nlohmann/detail/conversions/to_json.hpp#L267-L281`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/detail/conversions/to_json.hpp#L267-L281) | 有值写值，空状态写 `nullptr`                      | 序列化时保持双向映射，避免空值变成未定义字段   |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **空状态解引用是未定义行为。** `*opt` 不抛异常；只有 `value()` 会抛。
- **`optional<bool>` 的双重真值。** `if (opt)` 检查是否装载，`if (*opt)` 检查装载的值；两者不要混。
- **`value_or` 会复制默认值。** 对昂贵类型，`value_or(T{})` 即使有值也可能构造临时对象；用 `value_or` 的引用重载或先判断。
- **`optional<T>` 不传播 `const`。** `const optional<T>` 的 `operator*` 返回 `const T&`，但 `optional<const T>` 是另一种类型；不要用后者表达“只读值”。
- **比较不会短路求值。** `opt == expensive()` 在空状态下仍可能构造 `expensive()`；需要短路时先检查 `has_value()`。
- **`emplace` 后旧值不可恢复。** 构造函数抛异常时 `optional` 为空，不提供回滚。
- **`optional` 不是错误处理的全能替代。** 它不携带原因、不携带错误码；需要诊断信息时用 `expected` 或 `variant<T, Error>`。
- **`optional<T>` 要求 `T` 可析构。** 存储按值，`T` 的析构函数会在 `reset`、赋值与析构时调用；不要把需要特殊清理的资源直接塞进去。

## 库联动

- **fmt**：`formatter<std::optional<T>>` 在 `T` 可格式化时可用，输出 `optional(42)` 或 `none`。调试日志里能直接区分“空”和“装了 0”。
- **nlohmann/json**：`from_json` / `to_json` 在 C++17 下把 `nullopt` 与 JSON `null` 互转。`from_json` 的重载只在未启用隐式转换时提供；`to_json` 在 C++17 下始终提供。升级库版本或切换 `JSON_USE_IMPLICIT_CONVERSIONS` 时要确认实际走哪条路径。
- **spdlog**：日志参数直接支持可格式化类型；把 `optional` 交给日志前确认 `T` 可格式化，否则编译期报错。
- **与 `std::variant` 的分工**：`optional<T>` 只有“有/无”两态；`variant<T, E>` 可以表达多个具名状态。状态超过两个时不要用嵌套 `optional` 硬凑。

## 横向对比

| 方案                  | 空状态         | 携带错误原因 | 所有权 | 主要代价                         |
| --------------------- | -------------- | ------------ | ------ | -------------------------------- |
| `std::optional<T>`    | `nullopt`      | 否           | 值语义 | 解引用需检查；有额外空间         |
| `T*`                  | `nullptr`      | 否           | 不拥有 | 生命周期与所有权需额外约定       |
| `std::unique_ptr<T>`  | `nullptr`      | 否           | 独占   | 堆分配；不适合小值               |
| `std::variant<T, E>`  | 由替代类型表达 | 是           | 值语义 | 类型与访问代码更重               |
| `std::expected<T, E>` | 错误状态       | 是           | 值语义 | C++23 起；错误类型需设计         |
| 异常                  | 抛出           | 是           | —      | 异常路径成本与禁用异常场景不适用 |

跨标准演进：C++17 引入 `std::optional`；C++20 增加三路比较与 `constexpr` 改进；C++23 增加 `and_then`、`transform`、`or_else` 与 `monadic` 接口。C++26 未改变核心语义。
