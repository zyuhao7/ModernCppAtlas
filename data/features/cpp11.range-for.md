---
id: cpp11.range-for
title: 范围 for 循环
standard: C++11
kind: language
category: [语法, 容器]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.6", clang: "3.0", msvc: "17.0"}
tags: [range-for, 范围循环, 迭代器, 容器, 遍历]
alternatives: [下标循环, 迭代器循环, std::for_each, 标准算法]
proposal: N2930
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用 range-for 遍历映射并直接读取键值对，省去迭代器样板"
    evidence: [ev-mold-range-for-undef-errors-001]
  - id: c2
    statement: "用 range-for 对注册表中的每个 logger 执行同一动作"
    evidence: [ev-spdlog-range-for-formatter-002]
  - id: c3
    statement: "用 const auto& 的 range-for 只读遍历静态数组"
    evidence: [ev-abseil-range-for-literals-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

遍历容器时，传统写法要同时维护迭代器、终止条件和解引用：`for (auto it = v.begin(); it != v.end(); ++it)`。三个位置各自都可能写错，容器类型一换，声明也要跟着改。对数组还要单独处理长度。

范围 for 把“遍历整个范围”折叠成一个循环变量。写法是 `for (声明 : 范围)`，编译器把它展开成基于 `begin()` / `end()` 的迭代器循环。语言只需要范围能提供这两个函数；标准容器、数组、`std::initializer_list` 和满足要求的用户类型都可以直接使用。标准提案为 N2930。

## 基础用法

循环变量前写 `const auto &` 表示只读引用，写 `auto &` 可以修改元素，写 `auto` 会复制每个元素。

<!-- compile -->

```cpp
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main() {
  std::vector<std::string> words{"atlas", "modern", "cpp"};
  for (const auto &word : words)
    std::cout << word << ' ';
  std::cout << '\n';

  std::map<std::string, int> counts{{"a", 1}, {"b", 2}};
  int total = 0;
  for (const auto &entry : counts)
    total += entry.second;
  std::cout << total << '\n';
}
```

输出 `atlas modern cpp ` 与 `3`。`entry` 是 `std::pair<const std::string, int>` 的引用，映射的键不可修改。完整版本见 [`examples/cpp11/range_for.cpp`](../../examples/cpp11/range_for.cpp)。

## 进阶用法

循环变量的类型决定语义。遍历 `std::vector<std::string>` 时用 `auto` 会为每个元素复制一份字符串；大对象和不可复制类型应写 `const auto &` 或 `auto &`。遍历只读的 `const` 容器时，`auto &` 会推导成 `const T &`；如果元素类型本身是指针，还要区分“指针不可变”和“指向的对象不可变”。

范围表达式只求值一次。`for (auto x : make_vector())` 中的临时容器生命周期覆盖整个循环，因此直接遍历返回值是安全的。但循环变量不能用于删除元素：在循环体内 `push_back`、`erase` 或 `clear` 会使当前迭代器失效，行为未定义。需要边遍历边修改结构时，先用 `std::remove_if` 或显式迭代器。

范围 for 不提供下标。需要位置信息时，可以手写计数器，或在 C++23 中用 `std::views::enumerate` 配合结构化绑定。`break`、`continue` 与普通循环相同。

对代理迭代器（例如 `std::vector<bool>`）要小心：`auto value` 拿到的是代理对象而不是 `bool`，写 `bool value` 才会真正取值。需要引用语义时，显式写出元素类型。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                | 用法                                    | 为什么典型                             |
| ------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------- | -------------------------------------- |
| mold   | [`src/passes.cc#L1571-L1573`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/passes.cc#L1571-L1573)                                               | 遍历错误映射并读取键值对                | 循环体只关心元素，不关心迭代器         |
| spdlog | [`include/spdlog/details/registry-inl.h#L125-L129`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/registry-inl.h#L125-L129) | 遍历注册表中的 logger，逐个设置格式化器 | 对容器中每个元素执行同一动作           |
| abseil | [`absl/time/format.cc#L111-L117`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/time/format.cc#L111-L117)                                 | 只读遍历静态字面量表并匹配前缀          | `const auto&` 避免复制且表明不修改元素 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都处在生产路径上，分别覆盖映射、注册表容器和静态数组三种常见遍历对象。

## 陷阱与边界

**默认按值复制。** `for (auto x : v)` 会复制每个元素；大对象应写 `const auto &`，需要修改时写 `auto &`。

**循环中不能改变容器结构。** 删除、插入或清空元素会使迭代器失效，即使当前元素之前已经访问完毕，行为仍未定义。

**范围表达式只求值一次。** 这是与手写 `begin()` / `end()` 循环的重要差别，也意味着循环开始后再改变容器不会改变遍历范围。

**临时对象的生命周期有边界。** 直接遍历返回的临时容器是安全的；但如果范围表达式返回的是指向已销毁对象的引用，循环仍然会悬空。C++23 才补上部分返回引用场景的生命周期延长规则。

**`auto&` 绑定到代理迭代器可能失败。** `std::vector<bool>` 返回代理对象，绑定非 const 引用通常无法通过编译，应显式写 `bool`。

**没有下标并不总是缺点。** 需要索引时不要为了用 range-for 而在循环体内维护单独计数器；显式下标循环往往更清楚。

## 库联动

- **标准容器与字符串**：`std::vector`、`std::map`、`std::string`、`std::array` 都提供 `begin()` / `end()`，直接支持范围 for。
- **初始化列表**：`for (int x : {1, 2, 3})` 遍历 `std::initializer_list`，适合固定集合。
- **标准算法**：只读遍历且需要提前退出时用范围 for；需要变换、筛选或并行时交给 `std::for_each`、`std::transform`、`std::ranges` 算法。
- **mold / spdlog / abseil**：链接器、日志注册表和时间解析器都用范围 for 表达“对每个元素做同一件事”。

## 横向对比

| 写法            | 代码量 | 索引可用 | 修改元素 | 主要限制                 |
| --------------- | ------ | -------- | -------- | ------------------------ |
| 下标循环        | 中     | 是       | 是       | 需要 `size()`，易越界    |
| 迭代器循环      | 多     | 否       | 是       | 三个位置都可能写错       |
| 范围 for        | 少     | 否       | 是       | 不能在循环中改变容器结构 |
| `std::for_each` | 中     | 否       | 是       | 需要额外包含算法头文件   |

跨标准演进：C++11 通过 N2930 引入范围 for；C++20 的 ranges 让范围 for 可以直接遍历视图与管道结果，C++23 的 `views::enumerate` 补上了位置信息。
