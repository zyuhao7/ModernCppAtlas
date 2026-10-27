---
id: cpp11.constexpr
title: constexpr
standard: C++11
kind: language
category: [编译期计算, 性能]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.6", clang: "3.1", msvc: "19.0"}
tags: [constexpr, 编译期求值, 常量表达式, static_assert, 元编程]
alternatives: [宏, 模板元编程, 运行期函数, enum hack]
proposal: N2235
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用 constexpr 在编译期按类型参数计算表大小"
    evidence: [ev-mold-constexpr-entry-size-001]
  - id: c2
    statement: "用 constexpr 函数计算编码缓冲区长度"
    evidence: [ev-drogon-constexpr-base64-length-002]
  - id: c3
    statement: "用 constexpr 工厂函数在编译期构造常量对象"
    evidence: [ev-abseil-constexpr-duration-factory-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

要在编译期拿到常量，C++11 之前只有两条路：宏和模板元编程。宏没有类型、没有作用域，调试器看不见；模板元编程用递归特化表达计算，语法晦涩、错误信息难读。普通的 `const` 变量虽然可能被优化，但语言不保证它在编译期求值，不能用作数组维度或模板实参。

`constexpr` 让普通的函数和变量声明具备“可在编译期求值”的能力。函数体满足约束时，同一个函数既能在运行期调用，也能在需要常量表达式的上下文（数组维度、模板实参、`static_assert`、枚举初值）里被编译器求值。标准提案为 N2235。

## 基础用法

`constexpr` 函数用普通语法书写。给定常量实参时，编译器在编译期算出结果。

<!-- compile -->

```cpp
#include <array>
#include <iostream>

constexpr int square(int x) { return x * x; }

constexpr int sum_squares(int n) {
  return n == 0 ? 0 : square(n) + sum_squares(n - 1);
}

constexpr int kSize = sum_squares(3);

int main() {
  static_assert(square(4) == 16, "computed at compile time");
  std::array<int, kSize> table{};
  std::cout << kSize << ' ' << table.size() << '\n';
}
```

输出 `14 14`。`sum_squares(3)` 在编译期得到 `14`，直接用作 `std::array` 的长度；`static_assert` 同样在编译期校验。完整版本见 [`examples/cpp11/constexpr.cpp`](../../examples/cpp11/constexpr.cpp)。

## 进阶用法

C++11 的 `constexpr` 函数限制很严：函数体只能有一条 `return` 语句（可用三元运算符和递归表达分支），不能有局部变量、循环或 `switch`。这让它更像“带类型的宏”。C++14 放宽为允许局部变量、循环和多条语句，写法接近普通函数；C++17 起支持 `if constexpr` 与 `constexpr` lambda；C++20 允许 `constexpr` 函数内动态分配与 `try`。

`constexpr` 变量隐含 `const`，且必须在编译期初始化。`constexpr` 函数不保证一定在编译期执行：用运行期实参调用时它就是普通函数。需要“强制编译期、运行期调用即报错”时，C++20 的 `consteval` 更合适。`constinit` 则用于保证静态存储期变量的初始化发生在编译期。

模板里的 `constexpr` 常与 `static_assert` 配合：用编译期布尔量做断言，或用 `if constexpr` 剪除不合法分支。`constexpr` 也用于 `std::array` 大小、位运算掩码、字符串哈希等零运行期开销的常量。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                               | 用法                                   | 为什么典型                                   |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------- | -------------------------------------------- |
| mold   | [`src/mold.h#L593-L594`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/mold.h#L593-L594)                                                        | 按架构在编译期计算表头与表项大小       | 大小依赖编译期布尔量，直接参与数组维度和断言 |
| drogon | [`lib/inc/drogon/utils/Utilities.h#L344-L347`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/utils/Utilities.h#L344-L347) | 在编译期计算 base64 编码后的长度       | 长度只依赖入参，可与缓冲区分配共用同一公式   |
| abseil | [`absl/time/time.h#L537-L540`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/time/time.h#L537-L540)                                      | 在编译期构造时间单位的 `Duration` 常量 | 单位换算无副作用，适合常量初始化             |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖类型相关常量、纯函数与工厂函数三种用法。

## 陷阱与边界

**C++11 的函数体限制。** 只能单条 `return`，分支要写成三元表达式或递归；C++14 起才允许局部变量和循环。

**`constexpr` 不等于运行时一定更快。** 它只保证“可以在编译期求值”，运行期调用仍是普通函数；性能收益来自把计算挪到编译期，而不是函数本身。

**不是所有表达式都能常量求值。** 含未定义行为、读取非 constexpr 变量、动态分配（C++20 前）的表达式会退回运行期，或直接编译失败。

**递归过头会耗尽编译期栈。** constexpr 递归本质上仍是模板实例化式的展开，深度过大时编译会变慢甚至失败。

**`constexpr` 变量需要完整类型。** 编译期求值要求类型可见且字面量类型（literal type）满足规则，含未初始化成员或非平凡析构的类型不能直接用作 constexpr 变量。

**跨编译单元要 `inline`。** C++17 前在头文件里定义 `constexpr` 变量可能引发 ODR 问题，C++17 的 `inline constexpr` 才彻底解决。

## 库联动

- **`std::array`**：长度必须是常量表达式，`constexpr` 计算出的尺寸可直接作模板实参。
- **`<type_traits>`**：`std::integral_constant` 及其派生（`std::true_type` 等）是编译期布尔量的载体。
- **字符串与哈希库**：编译期字符串哈希、固定大小缓冲区都用 `constexpr` 消掉运行期初始化。
- **mold / drogon / abseil**：链接器、Web 框架与时间库都用 `constexpr` 把表格尺寸和单位换算挪到编译期。

## 横向对比

| 方案              | 类型安全 | 调试可见 | 可作模板实参 | 主要限制               |
| ----------------- | -------- | -------- | ------------ | ---------------------- |
| 宏                | 否       | 否       | 是           | 无作用域，易踩坑       |
| 模板元编程        | 是       | 难       | 是           | 语法晦涩，错误信息难读 |
| `constexpr` 函数  | 是       | 是       | 是           | C++11 函数体限制多     |
| 普通 `const` 变量 | 是       | 是       | 不一定       | 不保证编译期求值       |

跨标准演进：C++11 通过 N2235 引入 `constexpr`；C++14 放宽函数体；C++17 增加 `if constexpr` 与 constexpr lambda；C++20 增加 `consteval`、`constinit` 与 constexpr 动态分配。
