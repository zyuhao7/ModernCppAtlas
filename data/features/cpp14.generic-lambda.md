---
id: cpp14.generic-lambda
title: 泛型 lambda
standard: C++14
kind: language
category: [函数式, 模板]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.9", clang: "3.4", msvc: "19.0"}
tags: [generic lambda, auto 参数, 闭包, 模板, 回调]
alternatives: [函数模板, 手写仿函数, std::function, 函数重载]
proposal: N3649
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用 auto 参数的 lambda 充当处理多种区块类型的局部辅助函数"
    evidence: [ev-mold-generic-lambda-push-001]
  - id: c2
    statement: "用泛型 lambda 对不同整数宽度统一做哈希"
    evidence: [ev-mold-generic-lambda-hash-002]
  - id: c3
    statement: "用带参数包的泛型 lambda 遍历 tuple 并逐个绑定 SQL 参数"
    evidence: [ev-drogon-generic-lambda-binder-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 的 lambda 参数类型必须写死。要让一个回调同时处理 `int`、`std::string` 或任意用户类型，只能退回函数模板、手写仿函数，或用 `std::function` 加类型擦除。前两者把简单逻辑拆到离调用点很远的地方，后者带来运行期间接调用。

C++14 允许 lambda 参数写 `auto`。编译器为闭包生成一个带模板 `operator()` 的类，每个实参类型各实例化一份。写法仍然是一个内联表达式，泛型能力却和函数模板相同。标准提案为 N3649。

## 基础用法

参数位置的 `auto` 与函数模板的类型推导规则一致，引用与 `const` 需要显式写出。

<!-- compile -->

```cpp
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

int main() {
  std::vector<int> numbers{1, 2, 3};
  std::vector<std::string> words{"a", "bb"};

  auto print = [](const auto &value) { std::cout << value << ' '; };
  std::for_each(numbers.begin(), numbers.end(), print);
  std::for_each(words.begin(), words.end(), print);
  std::cout << '\n';
}
```

输出 `1 2 3 a bb `（末尾有一个空格）。`print` 是一个闭包对象，对它调用 `operator()<int>` 与 `operator()<std::string>` 会各自实例化。完整版本见 [`examples/cpp14/generic_lambda.cpp`](../../examples/cpp14/generic_lambda.cpp)。

## 进阶用法

`auto&&` 是转发引用的简写，能同时绑定左值和右值。需要原样转发时写 `[](auto &&value) { use(std::forward<decltype(value)>(value)); }`，只读访问则用 `const auto &`。

参数包同样适用：`[](auto &&...args)` 生成可变参数的模板调用运算符，配合 `std::apply` 或折叠表达式可以展开 tuple 与任意参数列表。

返回类型可以写 `decltype(auto)` 保留引用与值类别。普通 `auto` 会按值返回，可能丢掉引用或触发拷贝。

泛型 lambda 不能直接递归调用自己，因为闭包类型在定义完成前不可见。常见做法是把自身作为参数传入，或先用 `std::function` 包一层。C++23 的 deducing this 提供了更直接的写法。

每个实参类型都会生成独立实例。模板代码体积、编译时间和调试信息随类型数量增长，热路径上大量不同类型会显著增加二进制大小。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                 | 用法                                                  | 为什么典型                                   |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------- | -------------------------------------------- |
| mold   | [`src/passes.cc#L93-L97`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/passes.cc#L93-L97)                                                        | `auto *x` 参数的 lambda 登记多种 `Chunk` 派生类       | 同一段逻辑服务多种类型，省去重载             |
| mold   | [`src/icf.cc#L236-L238`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/icf.cc#L236-L238)                                                          | `auto val` 参数的 lambda 对任意整数宽度做哈希         | 只依赖字节表示，类型由调用点决定             |
| drogon | [`orm_lib/inc/drogon/orm/Criteria.h#L122-L127`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/inc/drogon/orm/Criteria.h#L122-L127) | `auto &&...args` 的 lambda 展开 tuple 并绑定 SQL 参数 | 参数包类型在调用点确定，泛型 lambda 负责展开 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。mold 的两处用于局部辅助函数，drogon 的一处用于延迟绑定，覆盖了泛型 lambda 最常见的两种角色。

## 陷阱与边界

**`auto` 参数按值推导。** `[](auto value)` 会复制实参，大对象应写 `const auto &` 或 `auto &&`。

**引用与 `const` 不会自动保留。** 与函数模板一样，传左值给 `auto` 参数得到副本，不会保持引用语义。

**每个类型一份实例。** 泛型 lambda 不是运行期多态；对不同类型调用会生成多份代码，类型越多编译越慢。

**不能直接递归。** 闭包类型在 lambda 体内尚不完整，需要显式传入自身或改用其它结构。

**重载决议可能出人意料。** 泛型 lambda 的 `operator()` 是模板，传入某些类型会参与模板推导并与其它重载竞争，错误信息通常较长。

**捕获列表不参与泛型。** `auto` 只影响参数；捕获的变量类型仍然固定，按值捕获会在构造闭包时复制。

## 库联动

- **标准算法**：`std::for_each`、`std::transform`、`std::visit` 等直接接受泛型 lambda，无需包装成函数对象。
- **mold**：链接器内部用泛型 lambda 处理多种区块和整数宽度，减少重复的辅助函数。
- **drogon ORM**：延迟绑定 SQL 参数，用参数包泛型 lambda 展开任意实参类型。
- **stdexec**：发送者适配器用模板 lambda 选择完成签名，把类型推导留在调用点。

## 横向对比

| 方案            | 定义位置       | 运行期开销 | 类型擦除 | 主要限制                     |
| --------------- | -------------- | ---------- | -------- | ---------------------------- |
| 泛型 lambda     | 调用点内联     | 无         | 否       | 每个类型实例化，不能直接递归 |
| 函数模板        | 命名空间作用域 | 无         | 否       | 逻辑远离调用点               |
| 手写仿函数      | 类作用域       | 无         | 否       | 样板代码多                   |
| `std::function` | 任意           | 有间接调用 | 是       | 可能堆分配，丢失内联机会     |

跨标准演进：C++14 通过 N3649 引入泛型 lambda；C++20 允许模板参数列表与约束，C++23 的 deducing this 让递归与显式对象参数更自然。
