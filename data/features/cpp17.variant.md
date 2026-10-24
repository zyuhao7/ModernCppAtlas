---
id: cpp17.variant
title: std::variant
standard: C++17
kind: library
category: [类型安全, 联合体, 值语义]
difficulty: 3
status: final
tier: A
compilers: { gcc: "7", clang: "4", msvc: "19.11" }
tags: [标签联合, 和类型, 访问模式, 异常安全]
alternatives: [继承 + 虚函数, 手写 union + 标签, std::any, std::optional, std::expected]
proposal: P0088R3
last_verified: "2026-10-25"
---

## 解决什么问题

“一个值可能是 A，也可能是 B”在 C 里通常写成 `union` 加一个手工维护的标签。`union` 不调用构造与析构，无法安全存放 `std::string` 这类非平凡类型；标签与成员也可能失配。面向对象写法用基类指针加虚函数，换来堆分配、间接访问和开放继承，但这里需要的是**封闭的备选集合**。

`std::variant<Ts...>` 是类型安全的标签联合：它在对象内部就地存放一个备选类型的值，并记录当前活动成员。类型标签由库维护，构造、复制、移动、析构都会正确转发给活动成员。它适合表达“解析结果要么是数字、要么是字符串”这类天然互斥的状态，而不是用一组可空字段互相约束。

## 基础用法

```cpp
#include <cstdint>
#include <string>
#include <variant>

using Value = std::variant<std::int64_t, std::string>;

Value parse(std::string_view text) {
  bool digits = !text.empty();
  for (char ch : text) digits = digits && ch >= '0' && ch <= '9';
  if (digits) return static_cast<std::int64_t>(std::stoll(std::string(text)));
  return std::string(text);
}

int main() {
  Value value = parse("42");
  if (auto *number = std::get_if<std::int64_t>(&value))
    std::cout << *number << '\n';
  std::cout << std::holds_alternative<std::string>(value) << '\n';
}
```

完整示例见 [`examples/cpp17/variant.cpp`](../../examples/cpp17/variant.cpp)：

```text
number 42
text atlas
number 7
42
1
0 1
1
bad_variant_access
```

示例用 `std::visit` 覆盖全部备选类型，用 `get_if` 做不抛异常的探测，再演示 `get<T>` 取错类型时的 `std::bad_variant_access`。

## 进阶用法

**访问用 `std::visit`。** `std::visit(visitor, value)` 要求 visitor 能接受每一个备选类型，返回值也必须在各分支间保持一致。常见写法是继承可变参数 lambda 的 `Overloaded` 辅助类型：

```cpp
template <class... Fs> struct Overloaded : Fs... {
  using Fs::operator()...;
};
template <class... Fs> Overloaded(Fs...) -> Overloaded<Fs...>;
```

这个辅助类型在 C++17 下即可使用；漏掉一个备选类型就是编译错误，而不是运行期遗漏。

**三种取值方式各有用途。** `std::get<T>(v)` 与 `std::get<I>(v)` 在类型不匹配时抛 `std::bad_variant_access`；`std::get_if<T>(&v)` 返回指针，不匹配时返回 `nullptr`，且不会抛异常。热路径、可预期失败用 `get_if` 或 `visit`；已经用 `holds_alternative` 确认过再用 `get`。

**转换构造的选择顺序可能反直觉。** `std::variant<std::string, bool> v = "hello";` 会选择 `bool`，因为 `const char *` 到 `bool` 是标准转换，而到 `std::string` 需要用户定义转换。想明确指定备选类型，用 `std::in_place_type<std::string>` 或先构造 `std::string`。

**赋值可能进入 `valueless_by_exception`。** 改变活动成员的赋值需要销毁旧值再构造新值；如果新值的构造抛出异常，`variant` 可能没有活动成员。此时 `index()` 返回 `variant_npos`，访问任何成员都抛 `std::bad_variant_access`。要保留强异常保证，先构造临时 `variant` 再移动赋值，或选用不抛异常的备选类型。

**默认构造要求第一个备选可默认构造。** `std::variant<int, std::string>` 可以默认构造为 `int{0}`；若第一个备选不可默认构造，需要显式初始化。想表达“空状态”时，把 `std::monostate` 放在首位，它不占额外语义。

**单个值不能重复备选类型。** `std::variant<int, int>` 非法；需要区分两个同类型值时，用 `struct` 包一层，或改用带标签的结构体。

**存储是内联的。** `variant` 的大小约为最大备选类型加一个索引，不会为备选值单独分配堆内存。备选类型必须是完整类型、对象类型，不能是引用、数组或 `void`；递归结构常借助 `std::vector` 间接持有自身。

## 真实项目证据

| 项目 | 位置 | 用法 | 为什么典型 |
|---|---|---|---|
| rui314/mold | [`src/cmdline.cc#L530-L544`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/cmdline.cc#L530-L544) | 解析 `--defsym` 右值，返回符号或数值地址 | 两个分支直接返回不同备选类型，由转换构造选择活动成员 |
| rui314/mold | [`src/passes.cc#L2908-L2921`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/passes.cc#L2908-L2921) | 先用 `get_if` 判断地址，再在 `else` 中取符号 | 非空指针同时完成类型判断与取值，另一分支可直接 `get` |
| rui314/mold | [`src/cmdline.cc#L1512-L1516`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/cmdline.cc#L1512-L1516) | 持有 variant 引用，用 `get_if` 过滤符号元素 | 只关心一个备选类型时不抛异常，数值地址成员自然跳过 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。mold 的构建配置声明 `cxx_std_20`，因此同一份代码也在 C++20 模式下编译。

## 陷阱与边界

- **`get<T>` 取错类型会抛异常。** 在 `try` 之外直接 `get`，等于假设自己已经知道活动成员；可用 `get_if` 的地方优先用 `get_if`。
- **隐式转换可能选错备选类型。** 字面量、`const char *`、算术类型之间的标准转换优先于用户定义转换；构造 `variant` 时显式写出类型或使用 `in_place_type`。
- **异常可能让 variant 变为 valueless。** 备选类型的移动构造若可能抛出，类型切换赋值就有风险；需要强保证时先构造临时对象。
- **`visit` 不是运行期多态。** 备选集合在编译期封闭；新增一种类型要改所有 visitor。需要开放扩展时，继承加虚函数更合适。
- **visitor 的返回类型必须一致。** 各分支返回 `int` 与 `double` 不会自动统一成公共类型；显式返回 `std::common_type_t` 或分别处理。
- **备选顺序影响默认构造与索引。** `index()` 从 0 开始按声明顺序编号；依赖数字索引的序列化格式在调整备选顺序后会失效。
- **variant 不是线程安全的。** 并发读写同一对象仍需外部同步；`holds_alternative` 后跟 `get` 之间没有原子性保证。
- **不要用 variant 代替错误类型。** “成功值或错误原因”用 `std::expected`（C++23）表达意图更清楚；`variant` 更适合并列的合法状态。

## 库联动

- **mold**：命令行解析把 `--defsym` 的右值统一存成 `variant<Symbol *, u64>`，链接阶段再按活动成员分派到符号解析或数值地址。
- **fmt**：`fmt/std.h` 为 `std::variant` 提供格式化支持；无法格式化某个备选类型时，编译期诊断会指向具体类型。日志里直接输出 variant 前，确认每个备选都满足 `formattable`。
- **nlohmann/json**：解析结果常需要在数字、字符串、对象之间分派；用 `std::visit` 把 variant 转成 `json`，比在每个调用点手写 `holds_alternative` 链更集中。
- **spdlog / 日志封装**：把结构化字段存成 variant，再由 visitor 统一序列化；避免在日志宏里对每种类型写重载。
- **与 `std::optional`、`std::expected` 的分工**：`optional` 表达“有或无”，`expected` 表达“成功或错误原因”，`variant` 表达“多个都合法的互斥状态”。

## 横向对比

| 方案 | 类型安全 | 存储位置 | 开放扩展 | 主要代价 |
|---|---|---|---|---|
| `std::variant<Ts...>` | 是 | 内联 | 否，改类型需重编译 | 大小取最大备选；异常可能 valueless |
| 手写 `union` + 标签 | 否 | 内联 | 否 | 必须手动管理构造、析构与标签一致性 |
| 基类 + 虚函数 | 是 | 通常堆上 | 是 | 间接调用、分配、生命周期管理 |
| `std::any` | 运行期检查 | 可能堆上 | 是 | 丢失备选集合，访问需精确类型 |
| `std::optional<T>` | 是 | 内联 | 单类型 | 只能表达有或无 |
| `std::expected<T, E>` | 是 | 内联 | 单值单错误 | 语义限定为成功/失败 |

跨标准演进：C++17 引入 `std::variant` 与 `std::visit`；C++23 的 `std::expected` 覆盖“值或错误”这一更窄但更常见的场景。当前代码若要按备选类型分派，仍要靠 `std::visit` 或 `get_if` 组合。
