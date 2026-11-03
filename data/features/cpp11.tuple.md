---
id: cpp11.tuple
title: std::tuple
standard: C++11
kind: library
category: [类型工具, 值语义]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.3", clang: "3.0", msvc: "16.0"}
tags: [std::tuple, 多返回值, 异构聚合, 结构化绑定, 字典序比较]
alternatives: [具名结构体, std::pair, std::optional, 输出参数]
proposal: N1403
last_verified: "2026-11-03"
claims:
  - id: c1
    statement: "用 tuple 一次返回名称、值与空值标志"
    evidence: [ev-abseil-tuple-split-name-value-001]
  - id: c2
    statement: "用 tuple 的字典序比较实现多字段排序键"
    evidence: [ev-mold-tuple-name-type-002]
  - id: c3
    statement: "用 tuple 给路由处理器的三个异构字段起别名"
    evidence: [ev-drogon-tuple-handler-info-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

函数需要返回多个值时，常见做法是定义只用于返回的具名结构体、写多个输出参数，或把结果塞进 `std::pair`。结构体最清楚，但为一次性返回值引入类型会增加样板；输出参数让调用点难以阅读；`pair` 只能容纳两个元素。

`std::tuple` 是任意数量、任意类型的值聚合。它把多个结果打包成一个可按值返回的对象，按编译期下标或类型取元素，并支持字典序比较。标准提案为 N1403。

## 基础用法

`std::make_tuple(...)` 推导元素类型并返回值；`std::get<I>(t)` 按下标访问，`std::tie(...)` 把变量绑定成引用元组用于解包，`std::ignore` 跳过不需要的位置。`std::tuple_size` 与 `std::tuple_element` 在编译期查询长度和元素类型。

<!-- compile -->

```cpp
#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>

std::tuple<std::string, int, bool> parse(const std::string& name, int score) {
  return std::make_tuple(name, score, score >= 60);
}

int main() {
  auto result = parse("alice", 82);
  std::cout << std::get<0>(result) << ' ' << std::get<1>(result) << ' '
            << (std::get<2>(result) ? "pass" : "fail") << '\n';

  static_assert(std::tuple_size<decltype(result)>::value == 3, "three fields");
  static_assert(
      std::is_same<std::tuple_element<1, decltype(result)>::type, int>::value,
      "score is int");

  int score = 0;
  bool passed = false;
  std::tie(std::ignore, score, passed) = parse("bob", 45);
  std::cout << score << ' ' << (passed ? "pass" : "fail") << '\n';

  auto extended = std::tuple_cat(result, std::make_tuple("extra"));
  std::cout << std::get<3>(extended) << ' '
            << std::tuple_size<decltype(extended)>::value << '\n';

  auto a = std::make_tuple(1, std::string("b"));
  auto b = std::make_tuple(1, std::string("c"));
  std::cout << (a < b ? "less" : "greater") << '\n';
}
```

输出 `alice 82 pass`、`45 fail`、`extra 4` 与 `less`。示例依次展示下标访问、编译期长度与元素类型检查、`tie`/`ignore` 解包、`tuple_cat` 拼接以及字典序比较。完整版本见 [`examples/cpp11/tuple.cpp`](../../examples/cpp11/tuple.cpp)。

## 进阶用法

**C++17 结构化绑定。** `auto [name, score, passed] = parse(...)` 直接声明三个变量，比 `std::tie` 更清楚，也不需要提前定义变量；`tie` 仍然适合给已有变量重新赋值。

**C++17 类模板实参推导。** 可以写 `std::tuple t{1, std::string("x"), true}`，元素类型由构造函数实参推导；C++11 中必须显式写 `std::tuple<int, std::string, bool>`。

**C++17 `std::apply`。** 把 tuple 展开成函数实参：`std::apply(fn, args)` 等价于 `fn(std::get<0>(args), ...)`，是实现通用调用包装的常用工具。

**tuple 作为排序键。** 元素类型都支持比较时，tuple 的 `operator<` 按元素顺序做字典序比较，正好对应多字段排序；这也是把多个字段打包成比较键的简洁写法。

**引用与生命周期。** `std::tie` 生成的是引用元组，适合解包已有变量；`std::make_tuple` 会按值复制，适合返回结果。`std::forward_as_tuple` 保留引用，只能在被引用对象存活期间使用。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                             | 用法                                    | 为什么典型                                                         |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------- | ------------------------------------------------------------------ |
| abseil | [`absl/flags/parse.cc#L284-L296`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/flags/parse.cc#L284-L296)                              | 用 tuple 返回名称、值与空值标志         | 三个异构结果一次返回，注释直接写出每个位置的含义，调用方按位置解包 |
| mold   | [`src/gdb-index.cc#L132-L135`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/gdb-index.cc#L132-L135)                                          | 用 tuple 的字典序比较实现多字段排序键   | 把三个字段装进 tuple 后直接比较，避免手写逐字段短路逻辑            |
| drogon | [`lib/inc/drogon/HttpAppFramework.h#L80-L80`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/HttpAppFramework.h#L80-L80) | 用 tuple 给路由处理器三个字段起类型别名 | `using` 别名明确表达用途，比裸 tuple 在接口中更易读                |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖多返回值、比较键和类型别名三种形态。

## 陷阱与边界

**`std::get<N>` 的下标必须是编译期常量。** 运行期下标无法使用 `get`；需要动态访问时说明数据不该用 tuple 表示，应改为数组或具名结构体。

**`std::tie` 绑定的是引用。** 用它解包时被绑定的变量必须已经存在；不要把 `tie` 的结果保存到比原变量更长的生命周期里。

**`std::ignore` 只用于解包。** 它跳过某个位置，不能反过来当作“可选值”使用；需要可选语义时用 `std::optional`。

**元素顺序是接口的一部分。** `get<1>` 的含义完全依赖调用方约定；公开 API 返回 tuple 时，顺序变化就是破坏性修改，具名结构体通常更稳妥。

**tuple 比较要求所有元素可比较。** 任一元素类型缺少 `operator<`，整个 tuple 的比较就无法实例化；错误信息通常很长，定位时先看第一个不满足的元素。

**不要把 tuple 当作轻量结构体滥用。** 元素多、字段含义复杂时，具名结构体的可读性和演进性更好；tuple 适合局部、短生命周期、含义由上下文明确的聚合。

## 库联动

- **`<tuple>`**：`make_tuple`、`get`、`tie`、`tuple_cat`、`tuple_size`、`tuple_element` 与 `apply` 都在此头文件。
- **`<utility>`**：`std::pair` 与 `piecewise_construct` 常与 tuple 配合，用于在 `map::emplace` 中原地构造键值。
- **`<functional>`**：C++17 的 `std::apply` 与 `std::invoke` 把 tuple 展开后调用，是通用包装器的基础。
- **abseil / mold / drogon**：分别展示多返回值、排序键与类型别名三种生产用法。
- **C++17 结构化绑定**：把 tuple 解包成具名变量，让 tuple 在调用点保持可读。

## 横向对比

| 方案         | 元素数量 | 元素类型 | 字段可读性 | 主要限制                         |
| ------------ | -------- | -------- | ---------- | -------------------------------- |
| 具名结构体   | 固定     | 任意     | 高         | 为一次性返回值引入类型，样板较多 |
| `std::pair`  | 2        | 任意     | 中         | 只能容纳两个元素                 |
| `std::tuple` | 任意     | 任意     | 依赖位置   | 字段无名字，顺序变化即破坏接口   |
| 输出参数     | 任意     | 任意     | 中         | 调用点可读性差，容易漏赋值       |

跨标准演进：C++11 通过 N1403 引入 `std::tuple`；C++14 放宽部分 `constexpr` 限制；C++17 增加类模板实参推导、结构化绑定与 `std::apply`；C++20 起 tuple 的比较与访问在常量表达式中更完整。
