---
id: cpp11.type-traits
title: 类型特征（type_traits）
standard: C++11
kind: library
category: [类型系统, 类型工具, 泛型编程]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.3", clang: "3.0", msvc: "16.0"}
tags: [type_traits, enable_if, is_same, conditional, SFINAE, 模板]
alternatives: [C++20 concepts, 手写特化, if constexpr, tag dispatch]
proposal: N1424
last_verified: "2026-11-03"
claims:
  - id: c1
    statement: "用 enable_if 与 is_same 做 SFINAE，检测类是否声明自动创建标志"
    evidence: [ev-drogon-traits-enable-if-autocreate-001]
  - id: c2
    statement: "用 conditional 与 is_same 把 void 映射成 false_type"
    evidence: [ev-drogon-traits-conditional-awaitable-002]
  - id: c3
    statement: "用 enable_if 与 is_same 分成两个重载，避免同类型转换时的多余 cast"
    evidence: [ev-spdlog-traits-conditional-static-cast-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

模板代码想根据类型做不同的事，最早只能写重载、偏特化或运行时 `typeid`。运行时判断既丢失编译期信息，也让错误推迟到运行期；手写特化则要为每个类型重复模板骨架，容易漂移。

`<type_traits>` 把类型属性变成编译期常量与类型：`is_integral<T>::value` 回答“是不是整型”，`enable_if` 让某个重载只在条件成立时存在，`conditional` 在两个类型之间选择，`decay`/`remove_reference` 把引用和 cv 限定剥掉。标准提案为 N1424。

## 基础用法

类型特征通常继承自 `std::true_type` 或 `std::false_type`，通过 `::value` 取布尔结果；变换类特征通过 `::type` 给出结果类型。`enable_if<B, T>` 只有在 `B` 为真时才定义 `type`，因此可以把条件放进函数返回类型或模板参数，让不满足条件的重载从候选集中消失。

<!-- compile -->

```cpp
#include <iostream>
#include <type_traits>

template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type twice(T value) {
  return value + value;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type twice(
    T value) {
  return value * 2;
}

template <typename T>
struct is_pointer_to_const : std::false_type {};

template <typename T>
struct is_pointer_to_const<const T*> : std::true_type {};

int main() {
  std::cout << twice(21) << ' ' << twice(1.5) << '\n';

  static_assert(
      std::is_same<int, typename std::decay<const int&>::type>::value, "decay");
  static_assert(
      std::is_same<int, typename std::remove_reference<int&>::type>::value,
      "remove reference");

  std::cout << std::is_same<decltype(twice(1)), int>::value << ' '
            << is_pointer_to_const<const int*>::value << ' '
            << is_pointer_to_const<int*>::value << '\n';

  using chosen = std::conditional<sizeof(void*) == 8, long long, int>::type;
  std::cout << std::is_integral<chosen>::value << ' '
            << std::is_convertible<int, double>::value << '\n';
}
```

输出 `42 3`、`1 1 0` 与 `1 1`。两个 `twice` 重载由 `enable_if` 互斥地限制在整型和浮点类型上；自定义的 `is_pointer_to_const` 通过偏特化识别 `const T*`；`decay`、`remove_reference` 和 `conditional` 展示类型变换。完整版本见 [`examples/cpp11/type_traits.cpp`](../../examples/cpp11/type_traits.cpp)。

`::value` 是 C++11 的取值方式；C++14 起多数特征提供变量模板 `_v`（如 `std::is_same_v<T, U>`），C++14 起变换类特征提供别名 `_t`（如 `std::decay_t<T>`）。

## 进阶用法

`enable_if` 可以放在返回类型、额外模板参数或函数参数上。返回类型写法最直观；额外模板参数写法能避免影响函数签名；函数参数写法会改变重载解析，通常不推荐。

`std::integral_constant` 是所有布尔特征的基类，也用于把编译期整数包装成类型。`std::bool_constant` 在 C++17 起是它的别名；自定义特征继承 `true_type`/`false_type` 后自动获得 `operator bool`、`operator()` 和 `value_type`。

类型变换分为两类：一类只改类型，如 `remove_cv`、`remove_reference`、`decay`、`conditional`；另一类回答关系，如 `is_same`、`is_convertible`、`is_base_of`。`decay` 同时处理数组到指针、函数到函数指针和顶层 cv，是转发场景中最常用的组合。

`enable_if` 的重载选择发生在模板实参推导阶段，失败是 SFINAE 而不是硬错误。C++17 的 `if constexpr` 可以在函数体内分支，减少重载数量；C++20 的 concepts 用约束语法表达同样的意图，错误信息更清晰。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                           | 用法                                                        | 为什么典型                                           |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------------- | ---------------------------------------------------- |
| drogon | [`lib/inc/drogon/DrObject.h#L60-L69`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/DrObject.h#L60-L69)               | 用 `enable_if_t` 与 `is_same_v` 检测类是否有自动创建标志    | 条件只在返回类型中成立，检测失败时回退到可变参数重载 |
| drogon | [`lib/inc/drogon/utils/coroutine.h#L58-L60`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/utils/coroutine.h#L58-L60) | 用 `conditional_t` 与 `is_same_v` 把 void 替换成 false_type | 把类型选择收敛到一个别名，调用方无需重复写条件       |
| spdlog | [`include/spdlog/common.h#L390-L399`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/common.h#L390-L399)                        | 用互斥的 `enable_if` 重载避免同类型转换时的多余 cast        | 两个重载条件互为补集，选择在编译期完成               |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖 SFINAE 检测、类型选择与重载分派。

## 陷阱与边界

**`enable_if` 写错位置会变成硬错误。** 条件不依赖函数模板参数时，替换失败不再是 SFINAE；确保条件里至少有一个待推导的模板参数。

**两个重载条件必须互斥。** 条件重叠会产生歧义，条件都不满足则没有可用重载；用 `!std::is_same<...>::value` 明确写出补集。

**`decay` 不只是去引用。** 它还会去掉顶层 cv、把数组转成指针、把函数转成函数指针；需要保留引用或数组时用更精确的 `remove_reference` / `remove_cv`。

**特征结果不一定是布尔。** `conditional`、`enable_if` 等变换类特征通过 `::type` 给类型；直接取 `::value` 会编译失败。

**C++11 没有 `_v` / `_t` 别名。** 变量模板和别名模板是 C++14 起逐步加入的；写 C++11 兼容代码时要使用 `::value` 和 `typename ...::type`。

**错误信息可能很长。** 深层 SFINAE 失败会打印整条重载候选链；C++20 concepts 能显著改善诊断。

**不要用类型特征做运行期分支。** `is_same<T, U>::value` 是编译期常量；运行期 `if` 仍会实例化两个分支，C++17 起应改用 `if constexpr`。

## 库联动

- **`<type_traits>` 与 `std::utility`**：`decay`、`enable_if`、`conditional` 常与 `std::move`、`std::forward` 组合，构成完美转发的基础。
- **C++17 `if constexpr`**：在函数体内按类型分支，减少为每个条件写一个重载。
- **C++20 concepts**：用 `requires` 约束替代大部分 `enable_if`，错误信息更可读。
- **drogon**：控制器注册与协程结果类型用类型特征做编译期分派。
- **spdlog**：格式化参数用 `enable_if` 排除与格式串冲突的重载。
- **Abseil / nlohmann/json**：在 C++11 兼容层中自带 `enable_if_t`、`decay_t` 等别名，升级标准后可切回 `std::`。

## 横向对比

| 方案              | 选择时机 | 错误信息   | 代码量 | 主要代价                           |
| ----------------- | -------- | ---------- | ------ | ---------------------------------- |
| 类型特征 + SFINAE | 编译期   | 较长       | 中     | 重载条件需要精心设计               |
| `if constexpr`    | 编译期   | 较好       | 少     | C++17 起可用，仍需类型特征提供条件 |
| concepts          | 编译期   | 最好       | 少     | C++20 起可用，需要编译器支持       |
| 运行时 `typeid`   | 运行期   | 不适用     | 少     | 丢失编译期信息，依赖 RTTI          |
| 手写模板特化      | 编译期   | 取决于写法 | 多     | 每个类型重复模板骨架               |

跨标准演进：C++11 通过 N1424 把 Boost 风格的类型特征纳入标准库；C++14 增加 `_t` 别名与变量模板 `_v`；C++17 增加 `bool_constant`、`conjunction`/`disjunction`/`negation`；C++20 的 concepts 在其上提供更直接的约束语法。
