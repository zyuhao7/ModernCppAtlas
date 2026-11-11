---
id: cpp11.algorithms
title: C++11 新增算法（all_of / any_of / copy_if）
standard: C++11
kind: library
category: [算法]
difficulty: 1
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "17.0"}
tags: [算法, all_of, any_of, none_of, copy_if, 谓词]
alternatives: [手写循环, std::find_if, C++20 ranges 算法, 自定义谓词函数]
proposal: N2666
last_verified: "2026-11-10"
claims:
  - id: c1
    statement: "环境变量布尔值用 std::any_of 匹配多个写法"
    evidence: [ev-nano-algorithms-any-of-env-001]
  - id: c2
    statement: "HTTP 版本号两段用 std::all_of 校验全为数字"
    evidence: [ev-glaze-algorithms-all-of-http-version-002]
  - id: c3
    statement: "initializer_list 构造函数用 std::all_of 判断形状"
    evidence: [ev-nlohmann-algorithms-all-of-init-list-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 之前判断“是否全部满足”“是否存在一个满足”只能手写循环，配一个提前 `return` 或 `break`。这类代码重复度高，循环边界与返回值容易写错，读代码时还要在脑中还原意图。

```cpp
bool all_even = true;
for (int value : values) {
  if (value % 2 != 0) {
    all_even = false;
    break;
  }
}
```

C++11 补充了一批谓词算法，把“全部”“任一”“都不”直接写进函数名：`std::all_of`、`std::any_of`、`std::none_of`，以及 `std::copy_if`、`std::find_if_not` 等。标准提案为 N2666。

## 基础用法

三个量化算法接收区间与一元谓词，返回 `bool`。

<!-- compile -->

```cpp
#include <algorithm>
#include <iostream>
#include <vector>

int main() {
  std::vector<int> values{2, 4, 6, 9};

  bool all_even = std::all_of(values.begin(), values.end(),
                              [](int value) { return value % 2 == 0; });
  bool any_large = std::any_of(values.begin(), values.end(),
                               [](int value) { return value > 5; });
  bool none_negative = std::none_of(values.begin(), values.end(),
                                    [](int value) { return value < 0; });

  std::cout << std::boolalpha << all_even << ' ' << any_large << ' '
            << none_negative << '\n';
}
```

输出 `false true true`。`9` 破坏了全偶条件，`6` 与 `9` 满足大于 5，区间内没有负数。完整版本见 [`examples/cpp11/algorithms.cpp`](../../examples/cpp11/algorithms.cpp)，五个场景的输出为：

```text
[1] all_even = false, any_large = true, none_negative = true
[2] all_of(empty) = true, any_of(empty) = false, none_of(empty) = true
[3] evens = 2 4 6
[4] first odd = 9 at index = 3
[5] all_lib = true, has_http = false
```

场景 2 验证空区间的返回值；场景 3 用 `copy_if` 配合 `std::back_inserter` 收集偶数；场景 4 用 `find_if_not` 拿到第一个奇数的值与下标，而不是只得到 `true`/`false`；场景 5 把 lambda 谓词用在字符串上，分别检查统一前缀与任一子串。

## 进阶用法

**空区间的定义要记住。** `all_of` 对空区间返回 `true`，`any_of` 返回 `false`，`none_of` 返回 `true`；这是“全称命题在空集上为真”的自然结果，却常让调用方意外。

**三者都会短路。** `all_of` 遇到第一个不满足谓词就返回 `false`，`any_of` 遇到第一个满足就返回 `true`；谓词不必对剩余元素执行。

**`copy_if` 过滤并复制。** `std::copy_if(first, last, out, pred)` 把满足谓词的元素写入输出迭代器，返回写入末尾的位置；配合 `std::back_inserter` 可直接追加到容器。

**`find_if_not` 是反向查找。** 它返回第一个不满足谓词的位置，适合“跳过前缀”的场景；与 `std::find_if` 成对出现。

**谓词可以是任意可调用对象。** 函数指针、lambda、函数对象都可以；需要捕获状态时用 lambda，谓词只应读取元素，不应修改区间。

**标准算法也接受执行策略（C++17）。** 同一组算法可以加 `std::execution::par` 并行执行，但谓词必须线程安全且不能有数据竞争。

## 真实项目证据

| 项目     | 位置                                                                                                                                                               | 用法                                 | 为什么典型                             |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------ | -------------------------------------- |
| nano     | [`nano/lib/env.cpp#L20-L31`](https://github.com/nanocurrency/nano-node/blob/6b5f63ce04c9a1ffe37b8c687fffc1bef941bf98/nano/lib/env.cpp#L20-L31)                     | 环境变量布尔值用 `std::any_of` 匹配  | 谓词直接表达“命中任一候选值”           |
| glaze    | [`include/glaze/net/http.hpp#L369-L372`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/net/http.hpp#L369-L372) | HTTP 版本号两段用 `std::all_of` 校验 | 同一谓词应用于两个子区间，省去重复循环 |
| nlohmann | [`include/nlohmann/json.hpp#L914-L923`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/json.hpp#L914-L923)        | 初始化列表用 `std::all_of` 判断形状  | 一次遍历判断所有元素是否为两元数组     |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖配置解析、协议校验与容器构造三种判定场景。

## 陷阱与边界

**空容器是真空真。** 用 `all_of` 校验配置项时，容器为空会直接得到 `true`；需要“至少一个元素”的语义时先判断 `empty()`，或改用 `any_of`。

**谓词顺序无保证。** 标准不承诺按迭代器顺序调用谓词；谓词有副作用或依赖执行顺序时，结果不可移植。

**谓词不能修改元素。** 算法可能复制谓词、也可能对元素取引用；在谓词里修改容器或元素会导致未定义行为或迭代器失效。

**返回值是迭代器或 `bool`。** `copy_if` 返回输出区间的末尾而不是容器大小；`all_of` 等返回 `bool`，不会告诉你第一个失败的位置，需要位置时用 `find_if_not`。

**并行策略改变要求。** 加上 `std::execution::par` 后，谓词会被多个线程并发调用，捕获的可变状态必须自行同步。

**MSVC 支持较晚。** 这批算法在 VS2012（17.0）前后才补齐；更老的工具链只能手写循环或使用 Boost.Range。

## 库联动

- **所有标准容器**：算法只依赖迭代器区间，`vector`、`list`、`deque`、`array` 与原生数组都能直接使用。
- **`std::find_if` / `std::find_if_not`**：需要位置而不只是布尔结果时，与量化算法配套使用。
- **`std::copy_if` + `std::back_inserter`**：过滤后追加到目标容器，避免预先计算大小。
- **Lambda 表达式**：谓词通常写成 lambda，捕获阈值或配置而无需单独的命名函数。
- **`std::execution`（C++17）**：执行策略把同一算法扩展到并行与向量化执行。
- **nano / glaze / nlohmann**：分别展示配置匹配、协议校验与容器构造三种判定。

## 横向对比

| 方案                            | 表达力       | 短路   | 主要限制                                 |
| ------------------------------- | ------------ | ------ | ---------------------------------------- |
| `all_of` / `any_of` / `none_of` | 高，意图直接 | 是     | 只返回布尔值，不给出失败位置             |
| 手写循环                        | 中           | 可控制 | 重复样板多，边界与提前返回容易写错       |
| `std::find_if`                  | 中           | 是     | 返回迭代器，需再与 `end()` 比较          |
| C++20 ranges 算法               | 高           | 是     | 需要 C++20，且 ranges 视图有生命周期约束 |

跨标准演进：C++11 通过 N2666 增加 `all_of`、`any_of`、`none_of`、`copy_if`、`find_if_not` 等算法；C++17 为大多数算法加入执行策略重载；C++20 把这些算法带入 `std::ranges` 命名空间，可以直接接收范围而不是一对迭代器，但谓词语义与短路行为保持一致。
