---
id: cpp17.fold-expressions
title: 折叠表达式
standard: C++17
kind: language
category: [模板, 可变参数, 编译期]
difficulty: 3
status: final
tier: A
compilers: {gcc: "6", clang: "3.6", msvc: "19.12"}
tags: [参数包, 编译期计算, 短路求值, 逗号折叠]
alternatives: [递归模板, initializer_list 展开, std::apply, 手写循环]
proposal: N4295
last_verified: "2026-10-25"
claims:
  - id: c1
    statement: "用带 true 初值的逻辑与折叠检查 tuple 中每个类型是否拥有资源"
    evidence: [ev-abseil-fold-owner-001]
  - id: c2
    statement: "用逗号折叠把 tuple 中的列名依次追加到 SQL 更新语句"
    evidence: [ev-drogon-fold-sql-update-003]
  - id: c3
    statement: "用逻辑或折叠在参数包中查找匹配的命名格式参数"
    evidence: [ev-fmt-fold-named-arg-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11/14 里，对参数包做“全部相加”“全部为真”这类归约，只能写递归模板：

```cpp
template <typename T>
bool all_true(T value) { return value; }

template <typename T, typename... Rest>
bool all_true(T value, Rest... rest) {
  return value && all_true(rest...);
}
```

每个操作都要一个基础重载，空包还需要单独处理；递归深度和模板实例化数量随参数增长。另一种 `initializer_list` 技巧（`int dummy[] = {0, (f(args), 0)...};`）只能做副作用，不能直接得到结果。

折叠表达式把“用二元运算符归约参数包”变成语言内建语法：编译器直接展开成 `E1 op E2 op ... op EN`，不需要辅助重载，也不再引入递归。

## 基础用法

```cpp
template <typename... Args>
auto sum(const Args&... args) {
  return (args + ... + 0);
}

template <typename... Preds>
bool all_true(Preds... preds) {
  return (preds && ...);
}

template <typename... Args>
void print_all(const Args&... args) {
  (std::cout << ... << args) << '\n';
}
```

四种形式：

| 形式                    | 展开含义                      | 空包结果                                              |
| ----------------------- | ----------------------------- | ----------------------------------------------------- |
| `(pack op ...)`         | `E1 op (E2 op (... op EN))`   | 仅 `&&` 得 `true`、`\|\|` 得 `false`、`,` 得 `void()` |
| `(... op pack)`         | `((E1 op E2) op ...) op EN`   | 同上                                                  |
| `(pack op ... op init)` | `E1 op (... op (EN op init))` | `init`                                                |
| `(init op ... op pack)` | `((init op E1) op ...) op EN` | `init`                                                |

完整示例见 [`examples/cpp17/fold_expressions.cpp`](../../examples/cpp17/fold_expressions.cpp)：

```text
sum: 15
sum empty: 0
all true: 1
all true empty: 1
any true: 1
any true empty: 0
joined: 1 2.5 atlas
total: 60
count: 3
```

示例同时演示了带初值的求和、`&&`/`||` 折叠、流输出折叠、逗号折叠，以及 `sizeof...` 作为“数参数”的对照。

## 进阶用法

**空包必须有明确的单位元。** 无初值的 `(args + ...)` 在空包时非良构；写成 `(args + ... + 0)` 才有零元。`&&`、`||`、`,` 三种运算符有标准规定的空包值，所以 `(preds && ...)` 可以直接接受空包。

**短路与求值顺序。** `&&`、`||`、`,` 折叠保留短路/顺序语义：`(preds && ...)` 遇到 `false` 停止，`(preds || ...)` 遇到 `true` 停止，逗号折叠按顺序执行每个表达式。其他运算符的求值顺序未指定；不要在折叠里依赖“先算左边”的副作用。

**二元折叠控制结合方向。** `(init + ... + pack)` 从左往右累加，`(pack + ... + init)` 从右往左。对加法这类满足结合律的运算结果相同；对字符串拼接、减法、流式输出，方向会影响结果与性能。`(std::cout << ... << args)` 是左折叠，输出顺序为书写顺序。

**逗号折叠适合副作用。** `(f(args), ...)` 依次调用 `f`，丢弃每个返回值；参数包为空时展开为 `void()`。它替代了 `initializer_list` 技巧，且没有“必须产生可初始化值”的限制。

**配合 `std::apply` 展开 tuple。** `std::apply([&](const auto&... item) { (use(item), ...); }, tuple);` 同时解决“取出 tuple 元素”和“遍历参数包”两件事，是处理聚合数据的常见组合。

**编译期判定。** `template <typename... Ts> constexpr bool all_integral = (std::is_integral_v<Ts> && ...);` 可以直接用于 `static_assert` 或约束。注意空包恒为 `true`，符合“全称命题对空集为真”的直觉。

**折叠的是表达式，不是语句。** 每个操作数必须是表达式；需要多条语句时，把它包进立即调用的 lambda 或逗号表达式。返回值类型由运算符决定，混用不同返回类型时可能触发意外的隐式转换。

**不要用折叠替代 `sizeof...`。** 计数本身不是归约，`sizeof...(Args)` 更直接且没有求值开销。

## 真实项目证据

| 项目                   | 位置                                                                                                                                                                                     | 用法                                                      | 为什么典型                                                        |
| ---------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------- | ----------------------------------------------------------------- |
| abseil/abseil-cpp      | [`absl/meta/type_traits.h#L603-L609`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/meta/type_traits.h#L603-L609)                              | `(true && ... && IsOwner<T>::value)` 检查 tuple 每个元素  | 用 `true` 作初值，空 tuple 直接得到 `true`，省掉递归偏特化        |
| fmtlib/fmt             | [`include/fmt/compile.h#L209-L216`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/compile.h#L209-L216)                                         | `(try_format_argument(...) \|\| ...)` 查找命名参数        | 逻辑或折叠在任意参数匹配时短路，空包返回 `false` 并走统一错误分支 |
| drogonframework/drogon | [`orm_lib/inc/drogon/orm/CoroMapper.h#L514-L518`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/inc/drogon/orm/CoroMapper.h#L514-L518) | `((sql += std::string(name) + " = $?,"), ...)` 拼更新语句 | `std::apply` 展开 tuple，逗号折叠按下标顺序追加列名，无中间容器   |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。前两条是编译期判定，第三条是运行期副作用；它们分别展示了折叠表达式最常见的两类用途。

## 陷阱与边界

- **忘记括号。** 折叠表达式必须整体加括号：`args + ...` 是语法错误，`(args + ...)` 才是表达式。
- **空包上的无初值折叠是非良构的。** 只有 `&&`、`||`、`,` 有规定的空包值；`(args + ...)` 在 `args` 为空时无法编译。模板是否会被空包实例化，需要在设计时就确定。
- **运算符可能被重载。** 折叠使用重载决议后的运算符。自定义 `operator+` 可能有副作用、返回代理对象或改变结合性；对库类型做折叠前先确认语义。
- **短路只对 `&&`、`||` 成立。** 其他运算符会求值所有操作数，即使前面的结果已经“决定”了答案。
- **不要依赖未指定的求值顺序。** `(f(xs) + ...)` 的调用顺序未规定；需要固定顺序时用逗号折叠或先收集到容器。
- **逗号折叠丢弃返回值。** 想保留结果必须让每个表达式写入共享状态（如引用参数），并自行保证不冲突。
- **类型推导容易变宽或变窄。** `sum(1, 2.5)` 的公共类型由 `operator+` 决定；有符号/无符号混合、窄整型提升都按普通表达式规则发生。
- **深层嵌套可读性差。** 折叠适合短小的归约；一旦需要多个步骤、提前退出或错误处理，写成普通循环或具名辅助函数更清楚。

## 库联动

- **Abseil**：`absl::IsOwner` 用折叠把类型萃取组合成对 tuple/variant 的整体判定，适合与 `std::bool_constant`、`static_assert` 组合。
- **fmt**：编译期格式串在 `constexpr` 路径里用逻辑或折叠逐个尝试命名参数，找不到时统一抛 `format_error`。
- **Drogon ORM**：`std::apply` + 逗号折叠把 tuple 的列名转成 SQL 片段，避免为每个字段写展开代码。
- **标准库组合**：`std::apply`、`std::tuple_size`、`std::index_sequence` 与折叠表达式经常一起出现；C++20 的 `std::ranges` 算法覆盖运行期集合，折叠表达式仍是编译期/异构参数包的工具。
- **日志与序列化**：需要按顺序输出或拼接时用左折叠 `(out << ... << args)`；需要逐项处理并共享状态时用逗号折叠。

## 横向对比

| 方案                    | 表达力             | 空包处理           | 编译期可用               | 主要代价                               |
| ----------------------- | ------------------ | ------------------ | ------------------------ | -------------------------------------- |
| 折叠表达式              | 任意二元运算符归约 | 由运算符或初值决定 | 是                       | 语法受限，深层逻辑可读性差             |
| 递归模板                | 任意逻辑           | 需专门基础重载     | 是                       | 实例化多、递归深度、代码冗长           |
| `initializer_list` 展开 | 仅副作用           | 天然支持           | 部分场景                 | 不能直接归约出值，易触发未使用变量警告 |
| `std::apply` + 手写循环 | 运行期 tuple 遍历  | 天然支持           | 否                       | 需要运行期容器或索引展开               |
| `std::accumulate`       | 同质序列归约       | 由初值决定         | 是（C++20 起 constexpr） | 只处理同类型序列，不能直接展开参数包   |

跨标准演进：折叠表达式由 N4295 引入 C++17，此后语义稳定。C++11/14 只能使用递归模板或 `initializer_list` 技巧；C++20 的 `concepts` 可以把折叠结果直接放进约束，C++26 的包索引（P2662）提供按下标访问参数包元素的另一条路径，但不替代折叠的归约用途。
