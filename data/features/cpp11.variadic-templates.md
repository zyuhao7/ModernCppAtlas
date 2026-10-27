---
id: cpp11.variadic-templates
title: 可变参数模板
standard: C++11
kind: language
category: [模板, 泛型]
difficulty: 4
status: final
tier: A
compilers: {gcc: "4.3", clang: "2.9", msvc: "18.0"}
tags: [variadic, 参数包, 模板, 完美转发, sizeof...]
alternatives: [C 风格变参, std::initializer_list, 递归重载, 宏展开]
proposal: N2242
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用模板参数包接收任意实参并完美转发给下层实现"
    evidence: [ev-spdlog-variadic-log-forward-001]
  - id: c2
    statement: "用参数包构造函数一次性转发实参给每个存储副本"
    evidence: [ev-drogon-variadic-io-storage-002]
  - id: c3
    statement: "用类型包与索引包同时展开成基类列表"
    evidence: [ev-abseil-variadic-compressed-tuple-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 之前，函数只能有固定数量的参数。要写一个接受任意参数个数的函数，只能用 C 的 `...` 变参：它丢掉类型信息，调用方和实现方靠约定对齐，读错类型就是未定义行为。模板也无法对类型个数做泛化，`std::tuple`、`std::function` 这类工具要么不存在，要么依赖大量手写重载。

可变参数模板引入“参数包”：`typename... Ts` 表示零个或多个类型，`Ts... args` 表示对应的一组值。函数模板、类模板都能用，配合完美转发可以把任意实参原样穿透多层调用。标准提案为 N2242，C++11 后期由 N2555 放宽了包在某些位置的展开限制。

## 基础用法

参数包可以按个数展开，也可以用递归逐个取出。`sizeof...(Ts)` 在编译期给出包内元素个数。

<!-- compile -->

```cpp
#include <cstddef>
#include <iostream>

std::size_t count_args() { return 0; }

template <typename T, typename... Rest>
std::size_t count_args(const T &, const Rest &...rest) {
  return 1 + count_args(rest...);
}

void print_all() { std::cout << '\n'; }

template <typename T, typename... Rest>
void print_all(const T &first, const Rest &...rest) {
  std::cout << first << ' ';
  print_all(rest...);
}

int main() {
  print_all(1, "two", 3.5);
  std::cout << count_args(1, "two", 3.5) << '\n';
}
```

输出 `1 two 3.5 ` 与 `3`。递归到空参数包时匹配无参重载，这是 C++11/14 处理参数包的经典写法。完整版本见 [`examples/cpp11/variadic_templates.cpp`](../../examples/cpp11/variadic_templates.cpp)。

## 进阶用法

完美转发是参数包最常见的用途：`template <typename... Args> void f(Args &&...args)` 里，`Args` 是转发引用，`std::forward<Args>(args)...` 按各自原始值类别传递。日志接口、工厂函数、`std::make_unique` 都靠这一模式把调用方实参无损交给底层构造函数。

展开位置决定语义。`f(args...)` 展开成逗号分隔的实参列表；`f(args)...` 是对每个元素分别调用后展开；`std::vector<int>{args...}` 展开成初始化列表。`sizeof...(args)` 与 `sizeof...(Args)` 都能取个数。C++17 的折叠表达式（`(args + ...)`）取代了大量递归，把求和、逻辑与等二元运算压缩成一行；C++17 的 `if constexpr` 也让递归的终止分支更清晰。

递归写法每个实参生成一层实例化，参数很多时编译开销大。需要高性能或深度展开时，优先用折叠表达式或对 `std::initializer_list` 展开。类模板里的参数包常用于实现 tuple、variant、函数签名萃取，`std::tuple_size`、`std::apply` 都建立在其上。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                              | 用法                                         | 为什么典型                                |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------- | ----------------------------------------- |
| spdlog | [`include/spdlog/logger.h#L78-L81`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/logger.h#L78-L81)                                               | 用参数包接收任意日志实参并完美转发           | 日志接口天然接受任意数量、任意类型的参数  |
| drogon | [`lib/inc/drogon/IOThreadStorage.h#L66-L70`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/IOThreadStorage.h#L66-L70)                    | 用参数包构造函数把实参转发给每个线程存储副本 | 一份构造签名同时服务多个线程实例          |
| abseil | [`absl/container/internal/compressed_tuple.h#L112-L121`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/container/internal/compressed_tuple.h#L112-L121) | 用类型包与索引包同时展开成多重继承基类列表   | 参数包按索引展开是 tuple 类实现的经典手法 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖函数转发、构造转发与类模板展开三种用法。

## 陷阱与边界

**递归写法的实例化深度。** 每个实参一层模板实例化，参数很多时会拉长编译时间、增大二进制；优先考虑折叠表达式。

**转发引用要配对 `std::forward`。** `Args &&` 的 `Args` 可能是左值引用类型，只写 `args...` 会丢失值类别，`std::move` 又会过度移动。

**包必须被展开使用。** 只声明 `typename... Ts` 而不展开会触发警告；`sizeof...(Ts)` 是唯一不需要展开的合法用法。

**空包要能匹配终止重载。** 递归方案必须提供一个无参重载，否则最后一个实参之后无法停止。

**展开顺序不保证。** 函数实参的求值顺序在 C++17 前未指定，现在仍不保证从左到右；有副作用的实参不要依赖顺序。

**不要用宏模拟参数包。** 现代编译器都支持真正的参数包，宏展开会失去类型检查和调试信息。

## 库联动

- **`std::tuple` / `std::variant`**：底层用参数包描述成员类型，`std::apply`、`std::visit` 负责展开。
- **`std::make_unique` / `std::make_shared`**：构造函数用参数包转发实参，避免为每种构造签名写重载。
- **日志与格式化库**：spdlog 用参数包接收任意日志参数，再转交给格式化后端。
- **并发与线程库**：`std::thread`、`std::async` 用参数包把可调用对象和实参打包进新线程。

## 横向对比

| 方案                    | 类型安全 | 参数个数 | 转发能力 | 主要限制                   |
| ----------------------- | -------- | -------- | -------- | -------------------------- |
| 可变参数模板            | 是       | 任意     | 完美转发 | 递归实例化，编译开销大     |
| C 风格 `...` 变参       | 否       | 任意     | 无       | 类型信息丢失，易未定义行为 |
| `std::initializer_list` | 是       | 任意     | 只读复制 | 要求同类型，不能转发引用   |
| 手写重载                | 是       | 固定     | 可以     | 每个参数个数都要写一份     |

跨标准演进：C++11 通过 N2242 引入可变参数模板；C++17 增加折叠表达式与 `if constexpr`，让参数包处理不再依赖递归；C++20 的 concepts 可以对包内类型施加统一约束。
