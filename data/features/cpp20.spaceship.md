---
id: cpp20.spaceship
title: 三路比较运算符 <=>
standard: C++20
kind: language
category: [比较]
difficulty: 3
status: final
tier: A
compilers: {gcc: "10", clang: "10", msvc: "19.20"}
tags: [比较, 排序, 运算符重写]
alternatives: [operator< 全家桶, std::tie, 手写 compare]
proposal: P0515R3
last_verified: "2026-10-24"
claims:
  - id: c1
    statement: "为 basic_json 实现三路比较并保留无序结果"
    evidence: [ev-json-spaceship-basic-json-003]
  - id: c2
    statement: "用 <=> 比较 JSON Pointer 的 token 序列"
    evidence: [ev-json-spaceship-pointer-001]
  - id: c3
    statement: "按预定义顺序比较值类型，无法比较时返回 unordered"
    evidence: [ev-json-spaceship-value-type-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++20 之前，一个可比较类型要写六个运算符：`==`、`!=`、`<`、`<=`、`>`、`>=`。它们容易彼此不一致：比如 `<` 按一个字段比较，`==` 按另一个字段比较，排序结果与等价判断互相矛盾。手写六个函数还意味着六处维护点，任何字段变动都要同步修改。

`<=>` 用一个三路比较返回“小于/等于/大于”的序关系，编译器据此重写其余关系运算符。配合 `= default`，成员逐一比较可以一行生成，且顺序与声明顺序一致。

## 基础用法

<!-- compile -->

```cpp
#include <compare>
#include <iostream>

struct Version {
  int major;
  int minor;
  int patch;

  auto operator<=>(const Version&) const = default;
};

int main() {
  Version a{1, 4, 0};
  Version b{1, 10, 0};
  std::cout << (a < b) << '\n';
  std::cout << (a == b) << '\n';
}
```

完整示例见 [`examples/cpp20/spaceship.cpp`](../../examples/cpp20/spaceship.cpp)：

```text
1
0
1
1.4.1 1.10.0 2.0.0
1
0 0 0
```

`a <=> b` 返回 `std::strong_ordering`；`a < b` 由编译器重写为 `(a <=> b) < 0`，`a == b` 由隐式声明的默认 `operator==` 处理。

## 进阶用法

**三种序关系对应不同语义。** `std::strong_ordering` 表示相等即不可区分（整数、默认逐成员比较的结构体）；`std::weak_ordering` 表示等价但可区分（大小写不敏感的字符串）；`std::partial_ordering` 允许“无序”（浮点 `NaN`）。强的可以隐式转弱的，弱的可以转偏序，反向不行。

**默认 `<=>` 会隐含默认 `==`。** 只写 `auto operator<=>(const T&) const = default;` 时，编译器同时声明默认 `==`。若自己写了 `==`，则不再生成。不要同时手写 `<=>` 和 `==` 却让两者比较不同字段。

**`<=>` 的返回类型不是 `bool`。** 返回 `bool` 会让 `a < b` 的重写语义失效，也丢失“相等/无序”的信息。自定义比较应返回三种序关系之一，或用 `std::strong_order` 等定制点把现有比较转成序关系。

**浮点成员会把结果降为偏序。** 默认逐成员比较时，只要有一个成员产生 `partial_ordering`（如 `double`），整个 `<=>` 就返回 `partial_ordering`；`NaN` 与任何值比较都是 `unordered`。用 `std::is_eq`、`std::is_lt`、`std::is_lteq` 等函数判断，而不是直接和 `0` 比较，可读性更好。

**手写 `<=>` 要比 `==` 更严格。** 约定是：`a <=> b == 0` 当且仅当 `a == b`。标准不强制检查，但违反约定会让排序容器与查找逻辑出现不一致。对大小写不敏感字符串，若 `==` 区分大小写而 `<=>` 不区分，应使用 `weak_ordering` 并让 `==` 与等价关系一致。

**重写只发生在运算符缺失时。** 如果类已经有 `operator<`，`a < b` 不会改走 `<=>`；如果已有 `operator==`，`a == b` 也不会由 `<=>` 重写。混用新旧运算符时，检查实际选中的是哪一个。

**枚举与指针也可以默认比较。** 枚举的 `<=>` 按底层值比较；指针的内置比较只保证同一数组内有序，跨对象结果未指定，需要全序时用 `std::less`。

## 真实项目证据

| 项目          | 位置                                                                                                                                                                                      | 用法                                                     | 为什么典型                                             |
| ------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------- | ------------------------------------------------------ |
| nlohmann/json | [`include/nlohmann/detail/json_pointer.hpp#L868-L872`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/detail/json_pointer.hpp#L868-L872) | 用 `<=>` 比较两个 JSON Pointer 的 token 序列             | 直接委托给已有容器的三路比较，不手写六个运算符         |
| nlohmann/json | [`include/nlohmann/detail/value_t.hpp#L95-L100`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/detail/value_t.hpp#L95-L100)             | 按预定义顺序比较 JSON 值类型，无法比较时返回 `unordered` | 偏序的典型用法：类型序不构成全序                       |
| nlohmann/json | [`include/nlohmann/json.hpp#L3745-L3754`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/json.hpp#L3745-L3754)                           | 为 `basic_json` 实现三路比较并保留无序结果               | 复杂类型的比较逻辑集中在一个入口，其余运算符由重写获得 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **不要返回 `int` 或 `bool`。** 三路比较应返回序关系类型；返回 `bool` 会让 `<`、`<=` 等重写产生错误语义。
- **默认比较按声明顺序。** 字段顺序改变会改变比较结果；把不影响身份的缓存字段放进比较会破坏等价关系。
- **`==` 与 `<=>` 可能不一致。** 编译器不检查两者的语义一致性；容器查找用 `==`，排序用 `<=>`，不一致时行为怪异。
- **浮点的 `unordered` 不是错误。** `NaN` 使比较返回 `unordered`，`a < b`、`a == b`、`a > b` 全为假；排序前要决定如何处理 `NaN`。
- **`<=>` 不是虚函数。** 通过基类引用比较时按静态类型选择；多态比较仍需虚函数或类型擦除。
- **重写有作用域限制。** 成员 `<=>` 只对同类型或可转换类型生效；异构比较需要额外重载或转换。
- **默认 `<=>` 要求所有成员可比较。** 含不可比较成员的类不能默认生成；手写时也要保证返回类型能表达所有成员的结果。
- **`std::strong_order` 不是 `<=>` 的替代品。** 它是定制点，用于把现有比较提升为指定序关系；不会自动为类生成运算符。

## 库联动

- **nlohmann/json**：`basic_json` 的 `<=>` 先比较类型序，再比较值；无法比较的组合返回 `partial_ordering::unordered`。这让 JSON 值可以参与排序，同时保留“不同类型不可比”的语义。
- **与 concepts 联动**：`std::three_way_comparable` 约束检查 `<=>` 的返回类型与一致性要求。用 concept 约束泛型排序函数，比在函数体里 `static_assert` 更早失败。
- **与 `std::set` / `std::map` 联动**：默认比较器仍使用 `std::less`，它通过 `<` 工作；`<=>` 让 `<` 自动生成，因此有序容器可以直接使用新类型。注意 `std::less` 不要求全序之外的一致性。
- **与 fmt / spdlog 联动**：序关系类型本身不可直接格式化；调试输出时用 `std::is_lt` 等转成布尔或字符串，避免打印出无意义的内部值。

## 横向对比

| 方案                | 需要实现的运算符        | 一致性           | 主要代价                           |
| ------------------- | ----------------------- | ---------------- | ---------------------------------- |
| `<=>` + `= default` | 1 个（`==` 可自动生成） | 成员顺序天然一致 | 需要 C++20；浮点成员得到偏序       |
| 手写六个运算符      | 6 个                    | 容易互相矛盾     | 维护点多，字段变动易漏             |
| `std::tie` 比较     | 每个运算符各写一次      | 字段顺序集中     | 仍需六个函数，且返回引用元组       |
| `compare` 成员函数  | 1 个 + 六个转发         | 逻辑集中         | 样板代码仍在，调用方要记返回值约定 |
| C++20 前的宏/CRTP   | 1 个宏 + 生成           | 取决于宏实现     | 可读性差，调试困难                 |

跨标准演进：C++20 引入 `<=>`、序关系类型与运算符重写；C++23 未改变核心语义。C++26 的反射提案可能自动生成比较，但当前仍应显式声明 `= default`。
