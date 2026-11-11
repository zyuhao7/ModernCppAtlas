---
id: cpp11.user-defined-literals
title: 用户定义字面量（user-defined literals）
standard: C++11
kind: language
category: [语法, 语言机制]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.7", clang: "3.1", msvc: "19.0"}
tags: [UDL, 字面量, 类型安全, 后缀]
alternatives: [工厂函数, 构造函数, 常量表达式, 预处理器宏]
proposal: N2765
last_verified: "2026-11-07"
claims:
  - id: c1
    statement: "用 operator\"\"_json 从字符串字面量直接构造 JSON"
    evidence: [ev-nlohmann-udl-json-001]
  - id: c2
    statement: "用 operator\"\"_a 创建命名格式参数"
    evidence: [ev-fmt-udl-named-arg-002]
  - id: c3
    statement: "用 operator\"\"_sql 包装自定义 SQL 字符串"
    evidence: [ev-drogon-udl-sql-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

单位、嵌入脚本、格式化参数这类值，写出来是字面量，类型却应该更具体：`12.5` 只是 `double`，调用方无法知道它是公里还是英里；`R"(...)"` 只是字符串，没法在类型层面区分 SQL 和普通文本。用工厂函数表达会多一层括号，用宏则失去类型检查。

用户定义字面量允许给字面量加后缀，让编译器在词法层面就把 `12.5_km`、`"..."_json` 转成用户类型。后缀是普通运算符（`operator""`），参与重载决议和命名空间查找，因此同时保留字面量的可读性和 C++ 的类型系统。标准提案为 N2765。

## 基础用法

数值字面量可以重载为 `unsigned long long` 或 `long double` 参数，字符串字面量接收 `const char*` 与长度。用户代码的后缀要以 `_` 开头，避免占用标准保留的名字。

<!-- compile -->

```cpp
#include <cstddef>
#include <iostream>
#include <string>

struct Kilometers {
  double value;
};

Kilometers operator""_km(long double value) {
  return Kilometers{static_cast<double>(value)};
}

std::string operator""_quoted(const char* text, std::size_t size) {
  return std::string("\"") + std::string(text, size) + "\"";
}

int main() {
  Kilometers distance = 12.5_km;
  std::cout << distance.value << '\n';
  std::cout << "atlas"_quoted << '\n';
}
```

输出 `12.5` 与 `"atlas"`。`12.5_km` 直接得到 `Kilometers`，`"atlas"_quoted` 在编译期已知长度，构造结果时不需要 `strlen`。完整版本见 [`examples/cpp11/user_defined_literals.cpp`](../../examples/cpp11/user_defined_literals.cpp)，四个场景的输出为：

```text
[1] 12_km = 12, 12.5_km = 12.5, type is Kilometers = 1
[2] "atlas"_length = 5
[3] json = {"ok":true}
[4] quoted = "atlas"
```

场景 1 同时提供 `unsigned long long` 与 `long double` 两个重载，整数与浮点字面量分别命中，`decltype` 确认结果类型就是 `Kilometers`。场景 2 的字符串重载直接拿到指针与长度，`_length` 返回 5，不需要运行时求长。场景 3 给原始字符串加 `_json` 后缀，JSON 文本在词法阶段就变成 `Json` 类型。场景 4 把运算符声明在 `atlas` 命名空间里，再用 `using` 引入到全局作用域。

## 进阶用法

**可用的参数列表是固定的。** 数值后缀接收 `unsigned long long` 或 `long double`，字符后缀接收 `char`、`wchar_t`、`char16_t`、`char32_t`，字符串后缀接收 `const char*`（或宽字符指针）加 `std::size_t`。不能自定义 `int` 或 `float` 参数。

**原始字面量模板。** 写成 `template <char...> operator""_x()` 可以逐字符接收数字，适合在编译期构造定长字符串或高精度常量；字符串版本则常用 `fixed_string` 非类型模板参数（C++20）做编译期解析。

**后缀的查找规则。** 字面量运算符是普通命名空间成员，通常需要 `using namespace` 引入，例如 `using namespace nlohmann::literals;` 之后才能写 `"..."_json`。标准库的 `std::chrono_literals` 也是同样机制。

**用户后缀必须以 `_` 开头。** 不含下划线的后缀保留给标准库（如 `s`、`ms`、`i`），含双下划线或“下划线加大写字母”的标识符在所有作用域都保留。

**后缀在词法阶段生效。** `-1_km` 会先解析 `1_km`，再对结果应用一元负号；如果后缀返回无符号类型，负数会得到意外结果。

**配合 `constexpr` 使用。** C++11 起字面量运算符可以是 `constexpr`，把解析工作前移到编译期；运行期只保留常量结果。

## 真实项目证据

| 项目     | 位置                                                                                                                                                                             | 用法                              | 为什么典型                                   |
| -------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------- | -------------------------------------------- |
| nlohmann | [`include/nlohmann/json.hpp#L5212-L5222`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/json.hpp#L5212-L5222)                  | `"..."_json` 解析 JSON 字面量     | 指针加长度直接解析，不构造临时字符串         |
| fmt      | [`include/fmt/format.h#L4150-L4161`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/format.h#L4150-L4161)                               | `"answer"_a=42` 命名格式参数      | 调用点顺序与格式串一致，比 `fmt::arg` 更直观 |
| drogon   | [`orm_lib/inc/drogon/orm/Criteria.h#L62-L68`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/inc/drogon/orm/Criteria.h#L62-L68) | `"..."_sql` 标记自定义 SQL 字符串 | 类型层面区分自定义 SQL 与参数化查询          |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖配置解析、格式化参数和 SQL 构造三种后缀语义。

## 陷阱与边界

**命名空间必须可见。** 字面量运算符不通过普通成员查找自动可见，忘记 `using namespace` 时编译报“没有匹配的字面量运算符”，错误信息通常只指向后缀。

**不要占用保留后缀。** 不以 `_` 开头的后缀保留给标准库，使用它们属于未定义行为；跨库组合时尤其容易撞名。

**字符串后缀拿到的是指针和长度。** 它不是 `std::string`，也不会替调用方校验内容；解析失败时的错误处理、编码假设都要在运算符里明确。

**负数与运算符优先级。** `-1_km` 等价于 `-(1_km)`；如果后缀返回自定义类型且没有一元负号，表达式可能直接编译失败，而不是得到负值。

**旧编译器对空格敏感。** 某些 GCC 版本要求写成 `operator "" _json`（运算符与后缀之间有空格），nlohmann 的证据里仍保留这条兼容分支；新代码按 `operator""_json` 写即可。

**运行期解析有成本。** 字符串后缀每次求值都会构造对象；高频路径应改成 `constexpr` 或缓存结果，而不是在每个循环里解析同一字面量。

**头文件里要 `inline`。** 非模板的字面量运算符定义在头文件中时必须 `inline`，否则多个翻译单元会违反 ODR。

## 库联动

- **`<chrono>`**：`std::chrono_literals` 提供 `1s`、`100ms`、`2h` 等后缀，是标准库最常用的字面量集合。
- **`<complex>`**：`1.0i` 构造虚数，展示数值后缀与运算符重载的配合。
- **`<string>` / `<string_view>`**：字符串后缀常返回 `std::string` 或 `std::string_view`，C++17 的 `sv` 后缀属于标准库。
- **fmt / nlohmann / mp-units**：分别用后缀表达命名参数、JSON 字面量和带单位的量。
- **drogon**：`_sql` 展示框架如何用后缀在 API 边界标记“这段文本是 SQL”。

## 横向对比

| 方案           | 调用点可读性 | 类型检查 | 运行期成本 | 主要限制                     |
| -------------- | ------------ | -------- | ---------- | ---------------------------- |
| 用户定义字面量 | 高           | 强       | 取决于实现 | 后缀命名空间、参数列表受限   |
| 工厂函数       | 中           | 强       | 与实现相同 | 多一层括号，字面量优势消失   |
| 构造函数       | 中           | 强       | 与实现相同 | `12.5` 无法直接携带单位语义  |
| 预处理器宏     | 低           | 无       | 无         | 展开后难调试，容易污染作用域 |

跨标准演进：C++11 通过 N2765 引入用户定义字面量；C++14 加入标准库的 `chrono` 字面量；C++17 引入 `string_view` 后缀；C++20 允许类类型非类型模板参数，使字符串字面量可以在编译期完整解析。C++23 继续补充标准库后缀，核心语法没有变化。
