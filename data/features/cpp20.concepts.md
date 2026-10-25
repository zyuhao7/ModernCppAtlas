---
id: cpp20.concepts
title: concepts 与 requires
standard: C++20
kind: language
category: [模板约束, 泛型编程]
difficulty: 4
status: final
tier: A
compilers: {gcc: "10", clang: "10", msvc: "19.23"}
tags: [约束, 模板, 重载决议]
alternatives: [SFINAE, enable_if, tag dispatch]
proposal: P0898R3
last_verified: "2026-10-24"
claims:
  - id: c1
    statement: "用 concept 包装已有的 is_formattable 特征"
    evidence: [ev-fmt-concepts-formattable-001]
  - id: c2
    statement: "用 requires 限制标量相等比较重载"
    evidence: [ev-json-concepts-scalar-eq-002]
  - id: c3
    statement: "同一标量约束复用于三路比较重载"
    evidence: [ev-json-concepts-scalar-spaceship-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++20 之前，模板对参数的要求只写在文档和错误信息里。给 `std::sort` 传一个不可比较的类型，编译器会在实例化深处抛出几十行错误；想限制重载候选，只能用 `std::enable_if` 把条件塞进返回类型或模板参数，签名迅速膨胀，错误信息仍然指向被剔除的候选而不是真正原因。

concepts 把“类型需要满足什么”写成具名约束，放在模板参数列表或函数声明上。约束参与重载决议，失败发生在候选筛选阶段；不满足时编译器能指出哪条约束失败，而不是从模板体内部报错。

## 基础用法

```cpp
#include <concepts>
#include <iostream>

template <typename T>
concept Incrementable = requires(T value) {
  { ++value } -> std::same_as<T&>;
  value++;
};

template <Incrementable T>
T advance_one(T value) {
  ++value;
  return value;
}

int main() {
  std::cout << advance_one(41) << '\n';
}
```

完整示例见 [`examples/cpp20/concepts.cpp`](../../examples/cpp20/concepts.cpp)：

```text
42
10
2.5
6
```

`requires(T value) { ... }` 不求值，只检查表达式是否合法、返回类型是否满足要求。`Incrementable` 作为约束写在模板参数上，不满足的类型不会进入函数体实例化。示例另外用 `Numeric` 约束普通函数，用 `requires` 子句约束范围算法。

## 进阶用法

**四类 requirement。** 简单要求写表达式（`value.size()`）；类型要求写 `typename T::value_type`；复合要求写 `{ expr } -> Concept`，同时检查表达式合法与返回类型；嵌套要求写 `requires OtherConcept<T>`，用于组合其他约束。复合要求里的 `noexcept` 也可以检查。

**约束可以组合并参与偏序。** `requires std::integral<T> && std::totally_ordered<T>` 是两个原子约束的合取；多个重载的约束满足子集关系时，更受约束的版本优先。`std::same_as<T, int>` 与 `std::integral<T>` 不是同一原子约束，不会互相替代。

**缩写函数模板更短。** `void f(std::integral auto x)` 等价于 `template <std::integral T> void f(T x)`。多个 `auto` 参数各自引入模板参数；需要引用同一类型时仍写显式模板参数。

**concept 只检查声明，不检查函数体。** 约束满足不代表调用一定能编译：函数体里的操作仍可能失败。concept 也无法表达“语义正确”，例如“比较必须传递”——这些是文档约定，不是类型系统能完全验证的。

**requires 表达式可以做编译期探测。** 它可以脱离 concept 单独使用，例如 `static_assert(requires(T t) { t.begin(); });`。但在非模板上下文中，表达式里的硬错误不会自动变成 `false`；需要依赖模板参数才会触发替换失败语义。

**concept 不能被特化。** 需要细化约束时，定义新的 concept 并在定义中引用旧的：`template<class T> concept Sized = std::ranges::range<T> && requires(T& t) { std::size(t); };`。不要试图给已有 concept 写特化。

**`requires` 子句与 `requires` 表达式是两件事。** 前者是写在声明上的约束（`template<class T> requires C<T>`），后者是产生布尔值的表达式（`requires(T t) { ... }`）。`requires requires` 是合法的，但通常拆成具名 concept 更易读。

**约束默认在返回类型之前求值。** `template<class T> requires C<T> auto f(T) -> R;` 中，`C<T>` 先于函数体；默认参数、`noexcept` 说明符里的约束顺序也要注意。

## 真实项目证据

| 项目          | 位置                                                                                                                                                            | 用法                                                        | 为什么典型                                                 |
| ------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------- | ---------------------------------------------------------- |
| fmtlib/fmt    | [`include/fmt/base.h#L2738-L2741`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/base.h#L2738-L2741)                  | 用 concept 包装已有的 `is_formattable` 特征                 | 在保留旧 trait 的同时给 C++20 用户提供约束接口，迁移成本低 |
| nlohmann/json | [`include/nlohmann/json.hpp#L3723-L3730`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/json.hpp#L3723-L3730) | 用 `requires std::is_scalar_v<ScalarType>` 限制标量比较重载 | 约束写在声明处，避免标量重载与容器重载产生歧义             |
| nlohmann/json | [`include/nlohmann/json.hpp#L3756-L3763`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/json.hpp#L3756-L3763) | 同一约束用于三路比较重载                                    | 多个运算符共享一致的标量约束，减少重复的条件编译           |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **concept 不保证语义正确。** 满足语法要求不代表满足数学性质；相等、排序、哈希的一致性仍需作者保证。
- **检查顺序影响诊断。** 原子约束按书写顺序替换；把最容易失败、最能说明问题的约束放前面，错误信息更短。
- **不要在 concept 里做重计算。** 每个候选、每次重载决议都可能实例化约束；复杂元编程会显著拖慢编译。
- **`requires` 表达式不求值，但有副作用类型检查。** 表达式中的类型必须可解析；未定义的类型名会导致硬错误，而不是 `false`。
- **concept 不能替代 `if constexpr`。** concept 控制“是否接受这个类型”，`if constexpr` 控制“在函数体内走哪条实现”。两者经常一起用，但职责不同。
- **缩写模板的 `auto` 可能产生意外重载。** `void f(Concept auto)` 与 `void f(T)` 的候选集不同；重载决议时确认约束确实参与了筛选。
- **不要用 concept 约束虚函数。** 虚函数不能是模板，约束也无法按动态类型选择；需要运行期多态时使用继承或类型擦除。
- **concept 名称与变量模板同名会冲突。** 同一命名空间里不要同时定义 `concept X` 和 `template<class> bool X`。

## 库联动

- **fmt**：`formattable` concept 建立在 `is_formattable` trait 上，C++20 用户可以用它约束自己的格式化包装，而不必依赖内部 trait；C++17 用户仍走旧路径。
- **nlohmann/json**：标量比较重载用 `requires` 限制 `ScalarType`，避免与数组、对象重载竞争；三路比较复用同一约束，保证 `==` 与 `<=>` 的候选集一致。
- **ranges**：`std::ranges::range`、`std::ranges::view`、`std::ranges::borrowed_range` 本身就是 concept，容器适配器与视图通过它们约束接口。自定义范围类型应满足这些 concept，而不是只提供 `begin` / `end` 成员。
- **与 SFINAE 的分工**：新代码优先用 concept 表达接口约束；需要兼容 C++17 或需要更细粒度的替换失败控制时，保留 `enable_if`。两者可以共存，但同一接口不要混用两套约束风格。

## 横向对比

| 方案                  | 约束位置           | 参与重载决议 | 诊断质量             | 兼容性   |
| --------------------- | ------------------ | ------------ | -------------------- | -------- |
| concepts / `requires` | 模板参数或声明     | 是           | 最好，指出失败约束   | C++20 起 |
| `enable_if`           | 返回类型或模板参数 | 是           | 差，候选被静默剔除   | C++11 起 |
| `static_assert`       | 函数体             | 否           | 中等，但已进入实例化 | C++11 起 |
| tag dispatch          | 重载参数           | 是           | 中等，需要额外类型   | C++03 起 |
| 文档约定              | 无                 | 否           | 无                   | 任意     |

跨标准演进：C++20 引入 concepts 与 requires 表达式；C++23 未改变核心规则；C++26 的扩展主要围绕标准库 concept 的补充，不改变用户定义与重载决议机制。
