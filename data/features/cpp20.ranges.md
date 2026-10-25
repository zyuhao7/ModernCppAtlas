---
id: cpp20.ranges
title: ranges 与 views
standard: C++20
kind: library
category: [范围, 惰性求值]
difficulty: 4
status: final
tier: A
compilers: {gcc: "10", clang: "15", msvc: "19.29"}
tags: [范围算法, 视图, 投影]
alternatives: [迭代器对算法, 手写循环, range-v3, Boost.Range]
proposal: P0896R4
last_verified: "2026-10-24"
claims:
  - id: c1
    statement: "为迭代代理特化 enable_borrowed_range"
    evidence: [ev-json-ranges-borrowed-001]
  - id: c2
    statement: "用 ranges concept 验证迭代器与视图属性"
    evidence: [ev-json-ranges-concepts-test-002]
  - id: c3
    statement: "用 views::reverse 反转 JSON 数组并比较"
    evidence: [ev-json-ranges-reverse-view-test-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

传统算法接收一对迭代器：`std::sort(v.begin(), v.end())`。这要求容器同时提供 `begin` / `end`，把容器传给算法要写两遍名字；处理“按某个字段排序”还要临时写比较器。视图组合（过滤、变换、反转）只能靠手写循环或第三方库，惰性求值更难表达。

ranges 把“范围”提升为一等概念：算法直接接收范围，支持投影（projection），视图可以按 `|` 组合且不复制数据。它解决的是**算法接口与惰性组合**问题，不改变容器本身的所有权模型。

## 基础用法

```cpp
#include <algorithm>
#include <iostream>
#include <ranges>
#include <vector>

int main() {
  std::vector<int> values{5, 1, 4, 2, 3};

  std::ranges::sort(values);
  for (int value : values | std::views::filter([](int v) { return v % 2 == 1; }) |
                            std::views::transform([](int v) { return v * 10; })) {
    std::cout << value << ' ';
  }
  std::cout << '\n';
}
```

完整示例见 [`examples/cpp20/ranges.cpp`](../../examples/cpp20/ranges.cpp)：

```text
10 30 50
5 4
7 14 21
test build deploy
deploy build
```

`std::ranges::sort` 直接接收容器；`views::filter` 与 `views::transform` 只构造视图，直到范围 `for` 迭代时才逐个计算。

## 进阶用法

**算法支持投影。** `std::ranges::sort(people, {}, &Person::age)` 按成员排序；`std::ranges::find_if(v, pred, proj)` 先把元素投影再判断。投影省掉临时比较器，但投影返回引用时要注意生命周期。

**视图是惰性的。** `v | views::transform(f)` 不立即调用 `f`，每次迭代才调用一次；`views::filter` 在寻找下一个匹配元素时可能多次调用谓词。把有副作用的函数放进视图，行为取决于迭代次数与是否提前退出。

**视图不拥有数据，除非它显式拥有。** `views::all(lvalue)` 通常产生 `ref_view`，引用原容器；把视图保存到容器销毁之后会悬垂。`views::all(rvalue)` 可以产生 `owning_view` 接管临时对象，但这只保证视图自身存活，不代表组合链中每一层都安全。

**`borrowed_range` 决定返回值是否可能悬垂。** 对临时容器调用 `std::ranges::find`，返回类型可能是 `std::ranges::dangling` 而不是迭代器；这是编译期保护。证据 1 就是为自定义视图特化 `enable_borrowed_range`，让标准库知道引用该视图的迭代器不会指向视图自身的临时存储。

**`filter_view` 的 `begin` 不是 const 的（C++20）。** 因为首次查找匹配元素可能需要缓存状态，C++20 的 `filter_view` 不能通过 const 对象迭代；需要只读遍历时，先物化结果或改用其他组合。

**不是所有视图都能排序。** `std::ranges::sort` 要求随机访问范围；`filter_view` 只提供前向/双向迭代，不能直接排序。需要排序时先 `ranges::to<std::vector>`（C++23）或手动复制。

**C++23 扩展了视图集合。** `views::zip`、`enumerate`、`chunk`、`slide`、`join_with`、`cartesian_product`、`repeat` 与 `ranges::to` 都在 C++23 加入。C++20 只有基础视图，跨标准写作时确认目标标准库实现。

**编译期成本可观。** 每个视图组合都会生成新类型，模板实例化与错误信息都比迭代器算法重。热路径上先测量，不要把长链视图当成零成本抽象。

## 真实项目证据

| 项目          | 位置                                                                                                                                                                                                                | 用法                                                        | 为什么典型                                                     |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------- | -------------------------------------------------------------- |
| nlohmann/json | [`include/nlohmann/detail/iterators/iteration_proxy.hpp#L232-L235`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/detail/iterators/iteration_proxy.hpp#L232-L235) | 为迭代代理特化 `enable_borrowed_range`                      | 让标准 ranges 算法知道该视图的迭代器可安全返回，避免误判为悬垂 |
| nlohmann/json | [`tests/src/unit-iterators2.cpp#L880-L894`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/tests/src/unit-iterators2.cpp#L880-L894)                                                 | 验证 JSON 迭代器满足 `bidirectional_range` 与 `input_range` | 用 concept 锁定容器与视图的范围属性，而不是只测运行时行为      |
| nlohmann/json | [`tests/src/unit-iterators2.cpp#L937-L946`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/tests/src/unit-iterators2.cpp#L937-L946)                                                 | 用 `std::views::reverse` 反转 JSON 数组并与期望值比较       | 展示视图组合与 `ranges::equal` 的典型配合                      |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。第 2、3 条来自测试，仅作补充；第 1 条是生产路径。

## 陷阱与边界

- **视图悬垂。** 保存引用局部容器的视图，在容器销毁后使用是未定义行为。
- **临时范围的返回值。** `ranges::find` 对临时非 borrowed 范围返回 `dangling`；不要强行解引用。
- **`filter_view` 的 const 限制。** C++20 下不能通过 const 对象迭代 `filter_view`；需要只读结果时物化。
- **谓词副作用。** 惰性求值次数不确定；谓词应保持无副作用，或明确接受重复调用。
- **投影返回临时对象。** 投影若按值返回，算法内部可能保存悬垂引用；返回成员引用或稳定对象。
- **无限范围配错算法。** `views::iota` 等无限范围只能与 `take`、`take_while` 等短路算法组合。
- **编译错误难读。** 长视图链失败时模板诊断很长；把链拆成具名变量，能定位到具体一层。
- **不要假定零成本。** 视图可能重复计算、缓存或产生复杂迭代器；性能敏感路径用基准测试比较手写循环。

## 库联动

- **nlohmann/json**：JSON 数组与对象迭代器满足 ranges concept，`items()` 视图通过 `enable_borrowed_range` 支持安全返回迭代器；因此 JSON 值可以直接参与 `views::transform`、`ranges::copy` 等组合。
- **fmt / spdlog**：`fmt::join` 可以格式化范围；把 `views::transform` 的结果交给 `join`，避免先物化中间容器。注意视图生命周期要覆盖格式化调用。
- **与 concepts 联动**：`std::ranges::range`、`view`、`borrowed_range` 是约束，不是运行时标签；自定义容器应满足这些 concept，才能享受算法与视图的通用接口。
- **与 `std::span` 联动**：`span` 是 contiguous、sized、borrowed 范围，可以直接进入视图链；它不拥有数据，生命周期仍由调用方负责。

## 横向对比

| 方案           | 接口形态        | 惰性组合   | 投影       | 主要代价                 |
| -------------- | --------------- | ---------- | ---------- | ------------------------ |
| ranges / views | 范围 + \`       | \` 组合    | 是         | 内置                     |
| 迭代器对算法   | `begin` / `end` | 否         | 手写比较器 | 冗长；容器名重复         |
| 手写循环       | 任意            | 取决于实现 | 手写       | 易出错；难以复用         |
| range-v3       | 范围 + 视图     | 是         | 内置       | 额外依赖；与标准版有差异 |
| Boost.Range    | 范围算法        | 部分       | 有限       | 依赖 Boost；接口较旧     |

跨标准演进：C++20 引入 ranges、views、投影与范围概念；C++23 增加 `views::zip`、`enumerate`、`chunk`、`slide`、`join_with`、`ranges::to` 等；C++26 继续补充视图适配器。使用新视图前先确认目标标准库的支持范围。
