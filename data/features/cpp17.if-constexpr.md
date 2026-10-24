---
id: cpp17.if-constexpr
title: if constexpr
standard: C++17
kind: language
category: [编译期分支, 泛型编程]
difficulty: 3
status: final
tier: A
compilers: {gcc: "7", clang: "3.9", msvc: "19.11"}
tags: [编译期, 模板, 分支]
alternatives: [SFINAE, tag dispatch, concepts]
proposal: P0292R2
last_verified: "2026-10-24"
---

## 解决什么问题

C++17 之前，模板里按类型走不同实现只有三条路：写多个重载做 tag dispatch、用 `std::enable_if` 做 SFINAE、或者把差异推给特化。三者都把“分支”放在函数签名或类模板层面，代码分散，错误信息难读。

普通的运行时 `if` 解决不了这个问题：两个分支都必须通过语义检查，模板参数相关的无效代码即使运行时不会执行，也会在实例化时报错。`if constexpr` 把条件放进函数体；在模板实体实例化时，如果条件在实例化后不再是值依赖的，未选中的分支不会被实例化，因此“只对某些类型成立”的代码可以留在同一个函数里。条件本身不必依赖模板参数。

## 基础用法

```cpp
template <typename T>
auto deref_or_value(const T& value) {
  if constexpr (std::is_pointer_v<T>) {
    return *value;
  } else {
    return value;
  }
}
```

`*value` 只在 `T` 为指针时实例化；传入 `int` 时，该分支被丢弃，不会产生“不能解引用 int”的错误。完整示例见 [`examples/cpp17/if_constexpr.cpp`](../../examples/cpp17/if_constexpr.cpp)：

```text
42
42
integer:42
floating:3.500000
integer:42
```

## 进阶用法

**是否检查丢弃分支取决于上下文。** 在非模板函数里，两个分支都要通过完整语义检查，`if constexpr (false)` 不能屏蔽非法代码：

```cpp
void f() {
  if constexpr (false) {
    int x = "not an int";  // 错误：非模板上下文仍检查 false 分支
  }
}
```

在模板实体实例化期间，只要条件在实例化后不再是值依赖的（字面量 `false`、`std::is_pointer_v<T>` 都满足），被丢弃的子语句就不会被实例化。所以条件不必直接依赖模板参数，另一个分支里的依赖表达式也不会被实例化：

```cpp
template <typename T>
void f() {
  if constexpr (false) {
    int x = "not an int";  // 合法：模板实例化时丢弃分支不实例化
  }
}

template <typename T>
void g() {
  if constexpr (true) {
    int x = 1;
  } else {
    typename T::missing value;  // 合法：else 分支被丢弃
  }
}
```

丢弃分支仍须是完整、可解析的 token 序列；它只是不参与实例化，不能用来替代 `#if` 处理编译器或平台差异。

**返回值类型可以按实例化分支不同。** 每个实例化只从实际参与返回的语句推导 `auto` 返回类型：

```cpp
template <typename T>
auto as_number(const T& value) {
  if constexpr (std::is_integral_v<T>) {
    return static_cast<long long>(value);
  } else {
    return static_cast<double>(value);
  }
}
```

传入整型得到 `long long`，传入浮点得到 `double`；不要求两个分支返回相同类型。

**和变参模板配合。** 用 `if constexpr (sizeof...(Args) > 0)` 保护递归终止条件，可以避免为空的参数包继续展开。证据 1 就是这种用法。

**区分编译期与运行期。** C++20 起可用 `std::is_constant_evaluated()` 在同一个函数里选择编译期实现；C++23 的 `if consteval` 语义更直接。两者解决的是“当前是否处于常量求值”，与 `if constexpr` 的“按类型选择代码”不同。

**不能替代约束。** `if constexpr` 位于函数体内，参与不了重载决议。若要让某个重载只在类型满足条件时存在，仍需 `requires`、`enable_if` 或特化。

## 真实项目证据

| 项目          | 位置                                                                                                                                                                                                        | 用法                                           | 为什么典型                           |
| ------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------- | ------------------------------------ |
| fmtlib/fmt    | [`include/fmt/compile.h#L99-L107`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/compile.h#L99-L107)                                                              | 参数包为空时跳过递归查找                       | 用编译期条件保护变参递归的终止路径   |
| fmtlib/fmt    | [`include/fmt/compile.h#L157-L166`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/compile.h#L157-L166)                                                            | 按 `is_named_arg` 选择读取 `.value` 还是原对象 | 编译期类型分派，避免运行期分支       |
| nlohmann/json | [`include/nlohmann/detail/input/binary_reader.hpp#L2821-L2835`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/detail/input/binary_reader.hpp#L2821-L2835) | 按大小与整型特性选择字节交换路径               | 丢弃分支无需对不支持的类型保持可编译 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **非模板上下文不丢弃分支。** 不在模板实体里实例化时，两个分支仍要做完整语义检查；用 `if constexpr (false)` 屏蔽非法代码是常见误用。
- **模板中的条件不必依赖模板参数。** 关键是条件在实例化后是否为值依赖；字面量条件同样可以让丢弃分支不实例化。丢弃分支仍须是合法的 token 序列。
- **作用域仍然存在。** 在分支内声明的变量不会泄漏到分支外；需要跨分支使用时，必须在 `if constexpr` 之前声明。
- **不参与重载决议。** 函数模板已经选中后才执行 `if constexpr`。类型不满足要求时，错误发生在函数体，而不是候选集筛选阶段。
- **返回类型推导依赖实例化。** 所有实际参与返回的语句必须能推导出一致的类型；被丢弃分支中的 `return` 不参与该次推导，但不要在非模板代码里依赖这一点。
- **不要把宏和它混用。** `#if` 在预处理阶段删除 token，能处理编译器/平台差异；`if constexpr` 仍要解析 token，只适合语言层面的类型与常量条件。
- **ODR 与内联。** 头文件中的 `if constexpr` 函数模板通常会实例化出多个版本；确保不同翻译单元的宏配置不会改变条件结果，否则容易出现 ODR 违规。

## 库联动

- **fmt**：编译期格式串解析大量使用 `if constexpr` 区分参数类型与命名参数。配合 `FMT_COMPILE` 时，分支结果直接决定生成的格式化代码路径；代价是模板实例化数量增加，编译时间和二进制体积都会上升。
- **nlohmann/json**：二进制解析在 `byte_swap` 中按 `sizeof` 与 `is_integral_v` 选择交换实现，避免为不支持 `std::byteswap` 的类型生成无效代码。适合与 `std::endian`、`std::bit_cast` 一起阅读。
- **与 concepts 的分工**：先用 `requires` 约束接口，再用 `if constexpr` 处理函数体内部的类型差异。把约束写在声明处，错误信息更短，也不会让不满足条件的类型进入实例化。

## 横向对比

| 方案                  | 生效位置        | 优点                           | 代价                               |
| --------------------- | --------------- | ------------------------------ | ---------------------------------- |
| `if constexpr`        | 函数体内部      | 写法集中，模板实例化时丢弃分支 | 不影响重载决议；非模板上下文仍检查 |
| SFINAE / `enable_if`  | 模板签名        | 参与重载决议，可精确控制候选集 | 签名膨胀，错误信息差               |
| tag dispatch          | 重载 + 标签类型 | 兼容 C++03，分支边界清楚       | 需要额外类型与转发层               |
| 类模板特化            | 类型定义层      | 适合按类型提供完整实现         | 样板代码多，组合困难               |
| concepts / `requires` | 声明约束        | 约束参与重载，诊断最好         | 需要 C++20；不能替代函数体内部署   |
| 运行时 `if`           | 执行期          | 条件可以是任意运行期值         | 两个分支都必须可编译，且有分支开销 |

跨标准演进：C++17 引入 `if constexpr`；C++20 增加 `std::is_constant_evaluated()`，可在同一函数中区分常量求值；C++23 增加 `if consteval`，把该判断写进语法。
