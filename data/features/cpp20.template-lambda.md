---
id: cpp20.template-lambda
title: 模板 lambda（template lambda）
standard: C++20
kind: language
category: [函数式, 模板]
difficulty: 3
status: final
tier: A
compilers: {gcc: "8", clang: "9", msvc: "19.22"}
tags: [lambda, 模板, 泛型, 参数包, 约束]
alternatives: [函数模板, 类模板仿函数, 泛型 lambda 的 auto 参数, std::function]
proposal: P0428R2
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "用带类型参数包的模板 lambda 在编译期选择完成签名并返回不同类型"
    evidence: [ev-stdexec-template-lambda-transform-001]
  - id: c2
    statement: "用 lambda 的模板参数展开下标序列，在编译期聚合完成信息"
    evidence: [ev-stdexec-template-lambda-completion-info-002]
  - id: c3
    statement: "把带 concept 约束的模板 lambda 放进重载集合，按约束选择候选"
    evidence: [ev-stdexec-template-lambda-awaitable-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++14 的泛型 lambda 用 `auto` 参数，能接受任意类型，但类型名不可见：没法直接声明参数包，没法给某个参数单独加约束，也没法在函数体内按类型做编译期分派。以前遇到这些需求，只能把逻辑搬到函数模板或类模板仿函数里，再用一个普通 lambda 转调。

C++20 允许 lambda 带模板参数列表：`[]<typename T>(T value) { ... }`。类型名在 lambda 体内可见，参数包、concept 约束和显式模板实参都能直接写，同时保留 lambda 的局部定义与捕获能力。标准提案为 P0428R2。

## 基础用法

模板参数列表写在捕获列表之后、参数列表之前。

<!-- compile -->

```cpp
#include <concepts>
#include <cstddef>
#include <iostream>
#include <vector>

template <typename T>
concept Sized = requires(const T &value) {
  { value.size() } -> std::convertible_to<std::size_t>;
};

int main() {
  auto sum = []<typename T>(T lhs, T rhs) { return lhs + rhs; };
  auto count = []<typename... Ts>(Ts...) { return sizeof...(Ts); };
  auto first_size = []<Sized T>(const T &value) { return value.size(); };

  std::cout << sum(3, 4) << ' ' << count(1, 2.0, 'x') << ' '
            << first_size(std::vector<int>{1, 2, 3}) << '\n';
}
```

`sum` 的两个参数必须推导成同一类型；`count` 用参数包统计实参个数；`first_size` 只接受满足 `Sized` 的类型。示例输出 `7 3 3`，完整版本见 [`examples/cpp20/template_lambda.cpp`](../../examples/cpp20/template_lambda.cpp)。

要绕过推导、显式指定模板实参时，必须写 `operator()`：

```cpp
auto make = []<typename T>(T value) { return value; };
auto value = make.operator()<int>(42);
```

在依赖上下文里还要加 `template`：`make.template operator()<int>(42)`。

## 进阶用法

模板参数列表可以同时带显式参数和 `auto` 参数，例如 `[]<typename T>(T first, auto second)`；`auto` 参数相当于追加在显式参数之后的模板参数。

需要一组按约束选择的重载时，不能直接给多个 lambda 取同一个名字，因为每个 lambda 表达式都有独立的闭包类型。常见做法是把若干模板 lambda 放进一个重载辅助对象，让重载决议在它们的调用运算符之间选择。stdexec 的 `__first_callable` 就是这种结构：一个候选要求 promise 支持 `await_transform`，另一个作为回退。

模板 lambda 还适合包装编译期分派：参数包里每个类型都参与 `if constexpr` 判断，分支返回不同类型时由 `decltype(auto)` 保留。传统写法需要把整个分派逻辑抽成函数模板，模板 lambda 可以把它留在使用点附近。

约束只作用在当前闭包类型上。两个各自带约束的模板 lambda 之间没有天然的偏序关系，重载集合仍要由外部辅助类型组织，这一点与普通函数模板重载不同。

## 真实项目证据

| 项目    | 位置                                                                                                                                                                                   | 用法                                                   | 为什么典型                                         |
| ------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------ | -------------------------------------------------- |
| stdexec | [`include/stdexec/__detail/__let.hpp#L546-L560`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__let.hpp#L546-L560)         | `[]<class... _As>(...)` 按完成签名做编译期分派         | 模板参数包 + `if constexpr` 集中在局部表达式里     |
| stdexec | [`include/stdexec/__detail/__let.hpp#L603-L613`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__let.hpp#L603-L613)         | `[]<std::size_t... _Is>(__indices<_Is...>)` 展开下标包 | 参数包直接来自 lambda 的模板参数，无需辅助函数模板 |
| stdexec | [`include/stdexec/__detail/__awaitable.hpp#L45-L51`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__awaitable.hpp#L45-L51) | 一个受 `__has_await_transform` 约束，另一个作为回退    | concept 约束写在 lambda 模板参数上，参与重载决议   |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在 stdexec 的生产头文件中，分别覆盖参数包分派、下标序列展开与约束重载。

## 陷阱与边界

**带模板参数列表的 lambda 总是泛型 lambda。** 即使参数类型全部写死，`operator()` 仍是函数模板；不同模板实参会生成独立实例，代码体积随调用组合增长。

**每个 lambda 表达式仍是独立类型。** 模板 lambda 不会让两个表达式共享闭包类型，也不能做显式特化或偏特化；需要重载集合时必须借助辅助对象。

**显式模板实参不能像函数调用那样简写。** 只能调用 `operator()`，在模板代码里还要写 `template` 消歧义。

**约束不跨闭包传播。** 一个模板 lambda 上的 concept 只约束它自己的调用运算符；把它和另一个 lambda 直接比较重载优劣没有意义。

**捕获与模板参数是两套东西。** 模板参数在编译期确定，捕获发生在运行期；按引用捕获局部变量后延长 lambda 的生命周期仍会悬空。

**闭包类型没有链接。** 模板 lambda 的实例化只在使用它的翻译单元内有效，不能把它当作可跨翻译单元引用的命名实体。

## 库联动

- **stdexec**：在发送者/接收者实现中用模板 lambda 做完成签名分派和约束重载，避免为每个局部分派点引入具名函数模板。
- **标准算法与 ranges**：泛型算法接受可调用对象，模板 lambda 可直接作为投影、比较器或变换函数，参数类型由算法调用时推导。
- **`std::function`**：类型擦除要求固定签名，模板 lambda 的多个实例化无法塞进同一个 `std::function`；只有先绑定成具体签名才可用。
- **函数模板**：需要跨翻译单元复用、显式特化或默认模板实参时，仍应写成具名函数模板。

## 横向对比

| 方案                  | 类型名可见 | 参数包 | 单参数约束 | 主要代价                           |
| --------------------- | ---------- | ------ | ---------- | ---------------------------------- |
| 泛型 lambda（`auto`） | 否         | 否     | 否         | 写法最短，复杂分派只能外移         |
| 模板 lambda           | 是         | 是     | 是         | 闭包类型不可复用，显式实参调用啰嗦 |
| 函数模板              | 是         | 是     | 是         | 必须定义在命名空间作用域，不能捕获 |
| 类模板仿函数          | 是         | 是     | 是         | 样板代码多，调用点需要构造对象     |

跨标准演进：C++14 引入泛型 lambda，C++20 通过 P0428R2 加入显式模板参数列表；此后 lambda 在模板能力上与函数模板基本对齐，但仍保留闭包类型不可命名、不可特化的限制。
