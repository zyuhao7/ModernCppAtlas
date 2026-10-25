---
id: cpp17.structured-bindings
title: 结构化绑定
standard: C++17
kind: language
category: [语法糖, 解构, 泛型编程]
difficulty: 2
status: final
tier: A
compilers: {gcc: "7", clang: "4", msvc: "19.11"}
tags: [解构, tuple, pair, 引用语义]
alternatives: [std::get, .first/.second, std::tie, 自定义访问器]
proposal: P0217R3
last_verified: "2026-10-25"
claims:
  - id: c1
    statement: "用结构化绑定接收 equal_range 返回的迭代器区间"
    evidence: [ev-mold-structured-bindings-equal-range-003]
  - id: c2
    statement: "用结构化绑定同时接收 insert 返回的迭代器与插入标志"
    evidence: [ev-mold-structured-bindings-insert-001]
  - id: c3
    statement: "把 to_chars 返回的指针与错误码绑定为具名变量"
    evidence: [ev-spdlog-structured-bindings-to-chars-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

从一个 `pair` 里取两个字段要写 `.first`、`.second`；从 `tuple` 里取要写 `std::get<0>`、`std::get<1>`；函数返回多个值时，调用方通常先声明变量，再用 `std::tie` 赋值。三种写法都把“字段是什么”藏在位置或名字约定里，读代码时要来回对照声明。

结构化绑定把多个名字放进同一个声明，直接绑定到表达式的子对象。它不创建新的聚合类型，也不改变函数返回类型；它只给已有的子对象起局部名字，并让你在声明处决定拷贝还是引用。

## 基础用法

```cpp
#include <iostream>
#include <map>
#include <string>
#include <tuple>

std::tuple<int, int> divmod(int value, int divisor) {
  return {value / divisor, value % divisor};
}

int main() {
  auto [quotient, remainder] = divmod(17, 5);
  std::cout << quotient << ' ' << remainder << '\n';

  std::map<std::string, int> counts;
  auto [it, inserted] = counts.insert({"apple", 1});
  std::cout << it->first << ' ' << inserted << '\n';
}
```

完整示例见 [`examples/cpp17/structured_bindings.cpp`](../../examples/cpp17/structured_bindings.cpp)：

```text
3 2
apple 1
1 0
amy 30 1
11 22
amy:95 bob:90
312
```

`auto [q, r]` 中的 `q`、`r` 不是新的 `tuple` 成员，而是隐藏对象内部子对象的别名。`insert` 返回 `pair<iterator, bool>`，绑定后可以直接用 `it` 和 `inserted`，不必记住哪个是 `.first`。

## 进阶用法

**声明形式决定拷贝还是引用。** `auto [a, b] = expr` 先拷贝整个 `expr`，绑定到副本；`auto& [a, b] = expr` 绑定左值引用，修改会写回原对象；`const auto& [a, b] = expr` 只读；`auto&& [a, b] = expr` 是转发引用，既能接左值也能接右值。遍历 `map` 时写 `for (const auto& [key, value] : map)` 可以避免每个元素复制一次 `pair<const Key, T>`。

**三种分解协议。** 数组按元素分解；`tuple`、`pair` 以及实现了 tuple-like 协议的类型，通过 `std::tuple_size`、`std::tuple_element` 与 `get<I>` 分解；普通类按非静态数据成员分解。成员分解要求这些成员属于同一个类（直接成员或同一个基类），并且可访问；派生类自己再添加成员后，绑定数量与布局都会变化，编译会直接失败。

**标识符数量必须完全匹配。** 不能只绑定前两个字段，也不能用占位符跳过。确实要忽略某个值，用 `std::tie` 配合 `std::ignore`，而不是结构化绑定。

**绑定名是子对象的别名。** `auto& [x, y] = point` 后写 `x = 10` 会修改 `point.x`；`auto [x, y] = point` 只修改副本。名字本身不能重新绑定到另一个对象，也不能在同一作用域重复声明。

**`map` 的键是只读的。** `for (auto& [key, value] : map)` 中 `key` 的类型是 `const Key`，可以改 `value`，不能改 `key`。想同时读写值又不想复制，这个形式最常用。

**自定义 tuple-like 类型。** 为自己的类型特化 `std::tuple_size` 与 `std::tuple_element`，并提供可被 ADL 找到的 `get<I>`，就能参与结构化绑定。特化必须在使用前可见；只为自己定义的类特化，不能为标准类型添加特化。

**分解只有一层。** `auto [a, b] = nested` 不会递归拆开嵌套的 `tuple`；需要继续分解时，对内层再写一次绑定。

## 真实项目证据

| 项目          | 位置                                                                                                                                                                        | 用法                                     | 为什么典型                                                                     |
| ------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------- | ------------------------------------------------------------------------------ |
| rui314/mold   | [`src/output-chunks.cc#L515-L518`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/output-chunks.cc#L515-L518)                             | 同时接收 `insert` 返回的迭代器与插入标志 | `pair<iterator, bool>` 的两个位置被直接命名，后续逻辑不依赖 `.first`/`.second` |
| gabime/spdlog | [`include/spdlog/details/fmt_helper.h#L33-L38`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/fmt_helper.h#L33-L38) | 分解 `to_chars` 返回的指针与错误码       | 成功分支用指针算长度，失败分支检查错误码，名字直接表达职责                     |
| rui314/mold   | [`src/input-files.cc#L1401-L1406`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/input-files.cc#L1401-L1406)                             | 接收 `equal_range` 返回的迭代器区间      | 绑定名 `begin`/`end` 让区间长度与返回语句保持可读                              |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三条都来自生产路径，没有使用测试代码充当主要证据。

## 陷阱与边界

- **`auto [key, value]` 会复制元素。** 在 `map` 或 `vector<pair<...>>` 的循环里，这会复制键和值；只读时用 `const auto&`，要修改时用 `auto&`。
- **`auto&` 不能绑定右值。** `auto& [a, b] = make_pair(1, 2)` 不合法；需要接临时对象时用 `auto&&` 或直接按值绑定。
- **`auto&&` 不延长所有子对象的生命周期。** 它绑定的是隐藏引用；把绑定名或引用返回出作用域仍会悬垂。需要长期保存时复制到普通变量。
- **不能跳过字段。** 结构化绑定没有占位符；忽略部分值要用 `std::tie` + `std::ignore`。
- **`map` 的键不能改。** `auto& [key, value]` 中 `key` 是 `const Key`；要修改键只能先删除再插入。
- **数量不一致是编译错误。** 数组、tuple 与成员的数量在编译期固定，不会在运行期截断。
- **成员分解不是任意反射。** 私有成员、多个基类各带数据成员、派生类与基类同时带数据成员，都会导致无法分解。
- **`decltype` 有特殊规则。** 不要把绑定名当成普通变量去推导类型；需要类型时显式声明，或对隐藏对象使用 `decltype`。
- **特化协议要完整。** 自定义 tuple-like 类型缺少 `tuple_size`、`tuple_element` 或 `get` 时，错误通常出现在实例化深处；先在最小示例里验证协议。

## 库联动

- **mold**：链接器大量处理 `insert` 返回值、`equal_range` 区间与符号表查找结果，结构化绑定把位置访问变成具名访问，减少读代码时的来回对照。
- **spdlog**：`to_chars` 的返回值被拆成指针与错误码，成功路径直接拿指针计算长度，失败路径读错误码，不需要临时 `pair` 变量。
- **fmt**：测试代码用 `const auto& [key, value] = result->values()` 读取扫描结果；库的 tuple-like 返回值可以直接参与绑定，测试只是补充证据。
- **nlohmann/json**：`items()` 的代理类型特化了 `tuple_size` / `tuple_element` 并提供 `get`，因此 `for (auto& [key, value] : j.items())` 可以直接遍历对象成员。
- **与 ranges 联动**：C++23 的 `views::zip` 产生 tuple-like 元素，循环里可以用结构化绑定同时取多个序列的当前值；生命周期仍由底层范围保证。

## 横向对比

| 写法                     | 拷贝/引用    | 能否跳过字段        | 适用对象                 | 主要代价         |
| ------------------------ | ------------ | ------------------- | ------------------------ | ---------------- |
| `auto [a, b] = e`        | 拷贝         | 否                  | 数组、tuple-like、聚合类 | 大对象会复制     |
| `auto& [a, b] = e`       | 左值引用     | 否                  | 左值                     | 不能接右值       |
| `const auto& [a, b] = e` | 只读引用     | 否                  | 左值与右值               | 不能修改         |
| `std::tie(a, b) = e`     | 引用赋值     | 是（`std::ignore`） | tuple-like               | 变量需预先声明   |
| `std::get<I>(e)`         | 取决于表达式 | 是                  | tuple-like               | 位置索引可读性差 |
| `.first` / `.second`     | 取决于表达式 | 是                  | `pair`                   | 只适用于两个字段 |

跨标准演进：C++17 引入结构化绑定；C++20 放宽了在 lambda 中捕获绑定名的限制；后续标准继续讨论把绑定扩展成参数包。当前代码若要在 C++17 与 C++20 之间复用，优先按 C++17 的限制写，避免依赖捕获规则。
