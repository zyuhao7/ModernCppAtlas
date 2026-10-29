---
id: cpp17.inline-variables
title: 内联变量
standard: C++17
kind: language
category: [语言机制]
difficulty: 2
status: final
tier: A
compilers: {gcc: "7", clang: "3.9", msvc: "19.12"}
tags: [ODR, 头文件, constexpr, 链接]
alternatives: [static constexpr, extern const, 函数返回引用]
proposal: P0386R2
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "在头文件中用 inline constexpr 定义多个翻译单元共享的 string_view 常量"
    evidence: [ev-drogon-inline-vars-pgstate-001]
  - id: c2
    statement: "用 inline constexpr 引用在头文件中提供兼容别名"
    evidence: [ev-abseil-inline-vars-inplace-002]
  - id: c3
    statement: "在头文件中用 inline constexpr 定义多个翻译单元共享的缓冲区大小常量"
    evidence: [ev-abseil-inline-vars-buffer-sizes-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

头文件里的命名空间级常量一直很尴尬。写 `constexpr int limit = 10;` 会得到内部链接：每个包含该头文件的翻译单元各有一份副本，取地址得到不同对象，跨翻译单元也无法共享同一实体。写 `extern const int limit;` 又必须在某个 `.cpp` 中补定义，头文件不再自足。

C++17 的 inline 变量允许在头文件中定义变量，并让所有翻译单元引用同一个实体。命名空间级变量、类静态数据成员都可以用 `inline`，不需要在 `.cpp` 中再写一遍定义。

## 基础用法

<!-- compile -->

```cpp
#include <iostream>
#include <string_view>

namespace config {
inline constexpr int kMaxRetries = 3;
inline constexpr std::string_view kServiceName = "atlas";
}  // namespace config

int main() {
  std::cout << config::kMaxRetries << ' ' << config::kServiceName << '\n';
}
```

`inline` 在这里不表示“把变量内联展开”，而是给变量 ODR 例外：允许多个翻译单元出现相同定义，链接器只保留一个实体。`constexpr` 保证常量初始化，`inline` 让定义可以安全地留在头文件中。

## 进阶用法

`inline` 最常出现在两个位置：命名空间级常量，以及类的静态数据成员。类内直接写 `static inline`，就不必在 `.cpp` 里再补一条定义。

```cpp
struct Limits {
  static inline int calls = 0;
  static constexpr int kMax = 3;
};

template <typename T> inline T cache;
```

`Limits::calls` 是可变的共享计数器，定义仍然只在头文件中出现一次；`kMax` 是 `constexpr`，C++17 起类内静态 `constexpr` 数据成员隐式 inline。模板的 `inline` 变量为每个实例化生成一个实体，`cache<int>` 与 `cache<double>` 互不相干。

同一翻译单元内取地址，得到的是同一个对象；跨翻译单元共享同一实体是标准保证，但动态库可能各自解析符号。需要稳定 ABI 时，不要把变量地址当作跨模块契约。

完整示例见 [`examples/cpp17/inline_variables.cpp`](../../examples/cpp17/inline_variables.cpp)。

## 真实项目证据

| 项目       | 位置                                                                                                                                                                                               | 用法                                                 | 为什么典型                                            |
| ---------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------- | ----------------------------------------------------- |
| Drogon     | [`orm_lib/src/postgresql_impl/PgConnection.h#L40-L50`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/src/postgresql_impl/PgConnection.h#L40-L50) | 在头文件中定义一组 SQLSTATE 常量                     | 多个翻译单元共享同一份错误码常量，定义不必搬到 `.cpp` |
| abseil-cpp | [`absl/utility/utility.h#L69-L75`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/utility/utility.h#L69-L75)                                              | 用 inline constexpr 引用绑定标准库的 `in_place` 对象 | 兼容别名需要头文件自足，同时保持单一实体              |
| abseil-cpp | [`absl/strings/numbers.h#L177-L179`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/strings/numbers.h#L177-L179)                                          | 在头文件中定义格式化缓冲区大小常量                   | 缓冲区上限被多个翻译单元使用，inline 变量免除类外定义 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。它们都把定义放在头文件中，而不是依赖单独的 `.cpp`。

## 陷阱与边界

**ODR 要求定义完全一致。** 所有翻译单元看到的 `inline` 变量定义必须相同；改初始值、类型或 `constexpr` 形态都要重新编译全部使用者。把宏展开进定义会让不同编译选项产生不同定义。

**`inline` 不等于常量。** `inline int counter = 0;` 是一个共享的可变对象，任何写入都需要同步。不要为了少写一个 `extern` 就把可变全局状态改成 inline 变量。

**动态初始化顺序仍不保证。** `inline constexpr` 走常量初始化；`inline` 非 constexpr 变量如果依赖运行时函数，初始化顺序跨翻译单元仍未指定。需要严格顺序时改用函数内静态对象或显式的初始化入口。

**跨动态库边界要谨慎。** 标准保证的是一个程序内的 ODR；动态库可能各自解析符号，不要假设可执行文件与插件一定共享同一地址。需要稳定 ABI 时，用函数返回引用，而不是导出变量地址。

**类静态成员的定义位置变了。** `struct S { static inline int value = 0; };` 不需要再写 `int S::value;`。若成员不是 `constexpr`，仍然要注意初始化顺序与线程安全。

**内部链接的旧写法没有消失。** 命名空间级 `const`、`constexpr` 默认仍是内部链接；加 `inline` 才变成外部链接的单一实体。不要把 `inline constexpr` 与 `static constexpr` 混为一谈。

## 库联动

- **Drogon**：PostgreSQL 连接实现把一组 SQLSTATE 错误码做成 `inline constexpr std::string_view`，多个翻译单元直接包含头文件即可使用。
- **Abseil**：`numbers.h` 共享缓冲区上限，`utility.h` 用 inline 引用别名标准库对象；两种写法都要求头文件自足，同时保持单一实体。
- **与 `constinit` 的分工**：`inline` 解决链接与 ODR，`constinit` 约束静态存储期变量必须常量初始化；C++20 代码可以写成 `inline constinit`，同时获得单一实体和初始化顺序保证。

## 横向对比

| 写法                                  | 链接与实体                         | 头文件自足 | 主要代价                             |
| ------------------------------------- | ---------------------------------- | ---------- | ------------------------------------ |
| `inline constexpr` 变量               | 外部链接，全程序一个实体           | 是         | 定义变化要求全部使用者重编译         |
| `static constexpr` / 命名空间级 const | 每个翻译单元一个内部链接实体       | 是         | 地址不唯一，跨翻译单元不能共享状态   |
| `extern const` 声明 + 一处定义        | 外部链接，一个实体                 | 否         | 需要额外的 `.cpp` 定义               |
| `inline` 非 const 变量                | 外部链接，一个可变实体             | 是         | 写入需要同步；动态初始化顺序仍不确定 |
| 函数内 `static`                       | 首次调用时初始化，C++11 起线程安全 | 是         | 惰性初始化，调用路径多一层检查       |

跨标准演进：C++17 引入 inline 变量，同时允许 `static constexpr` 数据成员隐式 inline。C++20 的 `constinit` 可以约束静态存储期变量必须常量初始化，但它不替代 `inline`；两者解决的问题不同。
