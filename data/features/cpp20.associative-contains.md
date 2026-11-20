---
id: cpp20.associative-contains
title: 关联容器的 contains
standard: C++20
kind: library
category: [容器]
difficulty: 2
status: final
tier: A
compilers: {gcc: "9", clang: "13", msvc: "19.21"}
tags: [contains, map, set, unordered_map, unordered_set, 键查找]
alternatives: [find 后比较 end, count 大于 0, at 加异常, 下标运算符, 自定义查表]
proposal: P0458R2
last_verified: "2026-11-18"
claims:
  - id: c1
    statement: "用 unordered_set::contains 判断请求路径是否已缓存"
    evidence: [ev-nghttp2-contains-path-cache-001]
  - id: c2
    statement: "在有序 map 的谓词里用 contains 判断 GNU property 键是否存在"
    evidence: [ev-mold-contains-gnu-properties-002]
  - id: c3
    statement: "在优化算子的断言里用 unordered_set::contains 校验元素未分配"
    evidence: [ev-vroom-contains-unassigned-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

在关联容器里判断“键是否存在”，C++20 之前只有两种写法：`find(key) != end()` 和 `count(key) != 0`。前者把一次成员测试写成迭代器比较，后者在 `multimap` / `multiset` 上还会遍历全部等值元素来计数，语义和开销都不直观。`at()` 能判断但按键缺失时抛异常，`operator[]` 会插入默认值，都不适合单纯的存在性检查。

P0458R2 给全部关联容器增加 `contains(key)`：返回 `bool`，不取迭代器，也不修改容器。它对 `map`、`set`、`multimap`、`multiset` 和四个无序版本都可用，把“键是否在容器中”从一个惯用法变成一个成员函数。

## 基础用法

`contains` 对所有关联容器语义一致：键存在返回 `true`，否则返回 `false`。

<!-- compile -->

```cpp
#include <iostream>
#include <set>
#include <string>

int main() {
  std::set<std::string> names{"alice", "bob"};
  std::cout << names.contains("alice") << ' '
            << names.contains("carol") << '\n';
}
```

输出 `1 0`。完整版本见 [`examples/cpp20/associative_contains.cpp`](../../examples/cpp20/associative_contains.cpp)，六个场景的输出为：

```text
[1] alice = 1, carol = 0
[2] 5 = 1, 6 = 0
[3] cached = 1
[4] spam = 1
[5] size = 3, carol = 60
[6] found = 1
```

场景 1 在 `std::map` 中检查两个键是否存在。场景 2 在 `std::set` 中检查整数。场景 3、4 分别在 `std::unordered_map` 与 `std::unordered_set` 上做同样的判断。场景 5 先用 `contains` 判断再插入缺失的键，避免覆盖已有值；这样写会查找两次，实际代码里一次完成判断和插入可用 `try_emplace`。场景 6 对比 `find`：需要取值或迭代器时仍用 `find`。

复杂度与 `find` 相同：有序容器为对数时间，无序容器平均为常数时间、最坏为线性时间。`contains` 不插入元素，也不会使迭代器失效。

## 进阶用法

**只关心存在性时用它。** 如果后续不需要元素本身，`find(key) != end()` 里的迭代器纯属负担；`contains` 直接返回 `bool`，条件表达式更短，读起来就是一次成员测试。

**写进谓词。** 需要按“键是否在某个集合里”过滤时，把 `contains` 放进 lambda 比先 `find` 再比较迭代器更清楚。链接器整理输出节时就用它判断某个键是否记录在映射中。

**插入前的守卫要看是否真的插入。** `if (!m.contains(k)) m.emplace(k, v);` 能表达“缺失才插入”，但会查找两次；`m.try_emplace(k, v)` 只查一次，键已存在时不构造值也不覆盖，见「std::map::try_emplace 与 insert_or_assign」页。`contains` 适合只判断、不插入，或判断后走完全不同分支的场景。不能用 `operator[]` 做这个判断，因为它会先插入默认值。

**支持透明查找。** 有序容器的比较器声明 `is_transparent`（如 `std::less<>`）时，`contains` 有异构重载，可以用 `std::string_view` 直接查 `std::string` 键，不构造临时字符串。无序容器要求哈希器和相等比较器都声明 `is_transparent`，这部分来自 P0919R3 与 P1690R1，支持版本更晚：GCC 11、Clang 12、MSVC 19.23。这一点与 `find` 的透明重载一致。

**断言与前置条件。** 调试断言里检查“这个键仍在待处理集合中”时，`assert(set.contains(key))` 比 `find` 版本短，失败信息也不依赖迭代器。

## 真实项目证据

| 项目                | 位置                                                                                                                                                                                                          | 用法                                        | 为什么典型                                   |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------- | -------------------------------------------- |
| nghttp2/nghttp2     | [`src/nghttp.cc#L1301-L1301`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/nghttp.cc#L1301-L1301)                                                                     | 用 `unordered_set::contains` 判断路径已缓存 | 只关心键是否存在，`find` 返回的迭代器用不上  |
| rui314/mold         | [`src/output-chunks.cc#L2783-L2783`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/output-chunks.cc#L2783-L2783)                                                           | 在谓词里用有序 `map::contains` 判断属性键   | 有序与无序容器共享同一接口，谓词只表达存在性 |
| VROOM-Project/vroom | [`src/problems/vrptw/operators/priority_replace.cpp#L67-L67`](https://github.com/VROOM-Project/vroom/blob/43dd7d0b8b560431eb555bf335cf4797eb7343c4/src/problems/vrptw/operators/priority_replace.cpp#L67-L67) | 用 `unordered_set::contains` 写前置条件断言 | 断言语句更短，且不需要比较迭代器             |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

**`contains` 只回答是否存在。** 它不返回元素或迭代器；需要键对应的值时用 `find()`、`at()`，需要区分“不存在”和“存在但值为空”时也不能靠它。

**对 `multimap` / `multiset` 不计数。** 只要至少有一个等值元素就返回 `true`；要知道出现次数时仍用 `count()`，要遍历所有等值元素用 `equal_range()`。

**透明重载需要比较器支持。** 默认 `std::less<Key>` / `std::hash<Key>` / `std::equal_to<Key>` 不接受异构键，`contains(string_view)` 会先构造 `Key`。有序容器需要比较器声明 `is_transparent`；无序容器需要哈希器与相等比较器同时声明，并且要 GCC 11、Clang 12、MSVC 19.23 及以上，比本页列出的 `contains` 最低版本更晚。

**C++17 及以前没有这个成员。** 需要用 `find(key) != end()` 或 `count(key) != 0`；没有专门的特性测试宏，按标准版本或编译器版本检测。

**序列容器不提供。** `vector`、`deque`、`list` 没有 `contains`；查找仍需算法 `std::find`，判断存在性要比较返回值与 `end()`。

**不要用它替代 `operator[]` 的插入语义。** `map::operator[]` 在键缺失时插入默认值；`contains` 只检测不插入，两者用途相反。

**返回值是 `bool`。** 不是迭代器，也不指示位置；复杂度和 `find` 相同，不存在额外性能损失。

**支持时间。** GCC 9、Clang 13、MSVC 19.21 起提供；提案为 P0458R2。

## 库联动

- **`find` / `end`**：通用键查找，返回迭代器；需要访问元素时用它。
- **`count`**：返回等值元素个数；对多重容器判断存在性也可以，但 `contains` 更直接。
- **`at`**：按键取值，缺失时抛 `std::out_of_range`，不适合存在性检查。
- **`try_emplace` / `insert_or_assign`**：插入与更新接口；“缺失才插入”直接用 `try_emplace`，一次查找完成，不必先调用 `contains`。
- **`std::erase_if`**：按谓词删除元素，谓词里可以用 `contains` 检查另一个容器。
- **无序容器页面**：`unordered_map` / `unordered_set` 的哈希与等价语义不变，`contains` 只增加一个成员。

## 横向对比

| 方案                 | 返回类型   | 是否插入 | 缺失时行为        | 主要限制                     |
| -------------------- | ---------- | -------- | ----------------- | ---------------------------- |
| `contains(key)`      | `bool`     | 否       | 返回 `false`      | C++20 起可用                 |
| `find(key) != end()` | 迭代器比较 | 否       | 返回 `end()`      | 写起来更长，迭代器通常用不上 |
| `count(key) != 0`    | 整数       | 否       | 返回 0            | 多重容器会遍历全部等值元素   |
| `at(key)`            | 值引用     | 否       | 抛 `out_of_range` | 用异常表达缺失，不适合纯判断 |
| `operator[](key)`    | 值引用     | 是       | 插入默认值        | 改变容器，不能用于存在性检查 |

P0458R2 把“键是否存在”从迭代器与计数惯用法中独立出来。它不改变容器结构或复杂度，但让存在性检查在有序与无序、单值与多值容器上拥有一致的名字。
