---
id: cpp11.uniform-initialization
title: 统一初始化（uniform initialization）
standard: C++11
kind: language
category: [初始化, 语法]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "16.0"}
tags: [花括号初始化, 窄化检查, 聚合初始化, initializer_list]
alternatives: [圆括号初始化, 赋值初始化, 工厂函数, C++20 指定初始化器]
proposal: N2672
last_verified: "2026-11-10"
claims:
  - id: c1
    statement: "时间字段用花括号统一构造"
    evidence: [ev-abseil-uniform-init-civil-fields-001]
  - id: c2
    statement: "解析结果结构体用聚合花括号初始化"
    evidence: [ev-fmt-uniform-init-parse-result-002]
  - id: c3
    statement: "字符串视图用花括号直接构造"
    evidence: [ev-spdlog-uniform-init-string-view-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 之前，初始化语法按类型分成好几套：标量用 `=`，类类型用圆括号，聚合体用花括号，数组又只能用 `=`。`T x(v)` 还可能被解析成函数声明（most vexing parse），`std::vector<int> v(10, 20)` 与 `std::vector<int> v{10, 20}` 的含义也完全不同。

```cpp
int value(3.9);                 // 静默截断成 3
std::vector<int> ten_twenties(10, 20);  // 10 个 20
// int narrowed{3.9};           // C++11 起编译错误
```

C++11 引入统一初始化（uniform initialization，又称花括号初始化）：`T object{args...}` 可用于所有类型。花括号初始化禁止窄化转换；当类型有 `std::initializer_list` 构造函数时优先选择它。标准提案为 N2672。

## 基础用法

花括号既能初始化聚合体，也能调用普通构造函数，还能填充标准容器。

<!-- compile -->

```cpp
#include <iostream>
#include <string>
#include <vector>

struct Point {
  int x;
  int y;
};

int main() {
  Point origin{};
  Point corner{3, 4};
  std::vector<int> values{1, 2, 3};
  std::string label{"atlas"};

  std::cout << origin.x << ' ' << origin.y << '\n';
  std::cout << corner.x << ' ' << corner.y << '\n';
  std::cout << values.size() << ' ' << values.front() << ' ' << values.back()
            << '\n';
  std::cout << label << '\n';
}
```

输出 `0 0`、`3 4`、`3 1 3` 与 `atlas`。空花括号把聚合体成员值初始化为零，`corner` 按声明顺序绑定 `3` 和 `4`，容器与字符串则走各自的构造函数。完整版本见 [`examples/cpp11/uniform_initialization.cpp`](../../examples/cpp11/uniform_initialization.cpp)，五个场景的输出为：

```text
[1] origin = 0 0
[2] corner = 3 4, returned = 5 6
[3] brace size = 2, paren size = 10, paren value = 20
[4] braced -> brace [1, 9], paren -> paren [1, 9]
[5] label = atlas, words = 3
```

场景 1 的 `Point origin{}` 是值初始化，两个成员都得到 `0`，不会读到未定义值。场景 2 展示聚合体按声明顺序绑定：`{3, 4}` 依次填入 `x`、`y`，`make_point` 用 `return {x, y}` 直接构造返回值。场景 3 是统一初始化最著名的语义分叉：`vector{10, 20}` 走 `initializer_list` 得到两个元素，`vector(10, 20)` 走数量构造得到十个 `20`。场景 4 用自定义 `Range` 放大同一现象：花括号优先匹配 `initializer_list` 构造函数（`brace`），圆括号匹配 `(int, int)` 构造函数（`paren`），两者数值相同但走的路径不同。场景 5 说明花括号同样适用于标准库类型：`std::string{"atlas"}` 避免圆括号被误读，`vector<string>{"a", "b", "c"}` 一次给出三个元素。窄化检查是编译期错误，因此只在源码注释里保留 `int narrowed{3.9}` 这类反例。

## 进阶用法

**窄化在编译期被拒绝。** `int x{3.9}`、`char c{1000}` 都是错误，而圆括号初始化会静默转换。这一条让花括号成为数值代码的默认选择。

**`initializer_list` 优先。** 类型一旦声明了 `std::initializer_list` 构造函数，`Type{...}` 会优先匹配它。`std::vector<int>{10, 20}` 是两个元素，`std::vector<int>(10, 20)` 才是十个 20。

**空花括号是值初始化。** `T{}` 对类类型调用默认构造函数，对聚合体把成员逐个零初始化；`int value{}` 得到 0，避免未初始化读取。

**聚合体按声明顺序绑定。** 聚合初始化不写字段名，顺序必须与声明一致；成员一旦有默认成员初始化器，C++11 下该类型不再是聚合体，C++14 才重新允许。

**可与 `=` 混用。** `T x = {1, 2}` 是拷贝列表初始化，语义与 `T x{1, 2}` 基本相同，但遇到 `explicit` 构造函数时会被拒绝。

**能包裹返回值。** `return Point{x, y};` 与 `return {x, y};` 都能避免临时对象的重复书写，fmt 的解析结果就采用这种写法。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                                                          | 用法                       | 为什么典型                                     |
| ------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------- | ---------------------------------------------- |
| abseil | [`absl/time/internal/cctz/include/cctz/civil_time_detail.h#L338-L349`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/time/internal/cctz/include/cctz/civil_time_detail.h#L338-L349) | 时间字段用花括号统一构造   | 多个对齐函数复用同一构造写法，字段顺序一目了然 |
| fmt    | [`include/fmt/compile.h#L329-L340`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/compile.h#L329-L340)                                                                              | 解析结果结构体用聚合初始化 | 成员按声明顺序绑定，省去手写构造函数           |
| spdlog | [`include/spdlog/common.h#L345-L348`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/common.h#L345-L348)                                                                       | 字符串视图用花括号直接构造 | 指针与长度显式传入，避免圆括号初始化的转换歧义 |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖聚合体、值类型与标准库视图三种初始化形态。

## 陷阱与边界

**`initializer_list` 会改变重载结果。** 最典型的坑是 `std::vector<int> v{10, 20}` 得到两个元素，而不是十个 20；需要数量语义时必须用圆括号。

**窄化检查也拦常量。** `int i{3.0}` 报错，即使值恰好可表示；浮点转整数必须显式 `static_cast`。

**空 `{}` 与 `{0}` 不同。** 对聚合体 `{}` 是值初始化，`{0}` 只初始化第一个成员，其余成员同样零初始化，但语义路径不同；对带 `initializer_list` 构造函数的类，`{}` 调用默认构造函数而非空列表构造函数。

**`auto` 推导会走样。** `auto x = {1, 2, 3}` 推出 `std::initializer_list<int>`；`auto x{1}` 在 C++11 推出 `initializer_list<int>`，C++17 起改为 `int`。跨标准代码不要依赖这条差异。

**聚合体的定义随标准变化。** C++11 的聚合体不能有默认成员初始化器，C++14 放宽；C++17 又允许聚合体有基类。同一段花括号代码在不同标准下可能合法或不合法。

**老工具链窄化检查不完整。** VS2010（16.0）已支持花括号初始化，但早期版本对窄化与 `initializer_list` 的边界行为有缺陷，升级前应实测。

## 库联动

- **`std::initializer_list`**：容器与自定义类型通过它接收花括号列表，统一初始化的另一半机制。
- **`std::vector` / `std::array` / `std::map`**：容器用花括号直接给出初始元素，嵌套写法保持可读。
- **`std::pair` / `std::tuple`**：值类型用花括号构造，配合结构化绑定更简洁。
- **聚合体配置**：`struct Options { int retries; bool verbose; };` 用 `Options{3, true}` 初始化，无需构造函数。
- **`std::string` / `std::string_view`**：花括号构造避免 `(ptr, len)` 被误读为其他重载。
- **abseil / fmt / spdlog**：分别展示值类型、聚合体与标准库视图三种花括号写法。

## 横向对比

| 方案         | 窄化检查   | 统一语法 | 主要限制                                |
| ------------ | ---------- | -------- | --------------------------------------- |
| 花括号初始化 | 有         | 是       | `initializer_list` 优先级可能改变语义   |
| 圆括号初始化 | 无         | 否       | 可能触发 most vexing parse              |
| 赋值初始化   | 无         | 部分     | 依赖隐式转换，`explicit` 构造函数不可用 |
| 工厂函数     | 由实现决定 | 是       | 需要额外函数与命名                      |

跨标准演进：C++11 通过 N2672 引入统一初始化与 `initializer_list`；C++14 允许带默认成员初始化器的聚合体；C++17 修改 `auto` 的花括号推导并引入类模板实参推导；C++20 增加指定初始化器，让聚合体可以按字段名初始化，但花括号的基本规则未变。
