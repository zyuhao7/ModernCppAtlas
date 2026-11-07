---
id: cpp11.unordered-set
title: std::unordered_set
standard: C++11
kind: library
category: [容器, 性能]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.1", msvc: "16.0"}
tags: [std::unordered_set, 哈希集合, 去重, 均摊常数时间]
alternatives: [std::set, std::vector, absl::flat_hash_set, bloom filter]
proposal: N1456
last_verified: "2026-11-06"
claims:
  - id: c1
    statement: "连接池用 unordered_set 维护连接集合"
    evidence: [ev-drogon-unordered-set-connections-001]
  - id: c2
    statement: "缓存桶用 unordered_set 去重回调"
    evidence: [ev-drogon-unordered-set-callbacks-002]
  - id: c3
    statement: "链接选项用 unordered_set 做成员去重"
    evidence: [ev-mold-unordered-set-options-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

需要判断“某个值是否出现过”、对输入去重，或维护一组对象时，`std::vector` 的线性查找在元素变多后成为瓶颈；`std::set` 虽然查找是对数时间，但每次插入都要维护红黑树的顺序，额外付出比较和节点分配成本。

`std::unordered_set<Key>` 用哈希表提供平均 `O(1)` 的插入、删除和查找，不保证元素顺序。它适合成员检查、去重、已访问集合这类只关心“在不在”的场景。标准提案为 N1456，头文件是 `<unordered_set>`。

## 基础用法

`insert` 返回 `pair<iterator, bool>`，`second` 表示是否真正插入；`count` 只返回 0 或 1；`find` 返回迭代器。自定义类型需要提供哈希和相等函数。

<!-- compile -->

```cpp
#include <cstddef>
#include <iostream>
#include <unordered_set>

struct Point {
  int x;
  int y;
};

struct PointHash {
  std::size_t operator()(const Point& point) const {
    return std::hash<int>()(point.x) ^ (std::hash<int>()(point.y) << 1);
  }
};

struct PointEqual {
  bool operator()(const Point& lhs, const Point& rhs) const {
    return lhs.x == rhs.x && lhs.y == rhs.y;
  }
};

int main() {
  std::unordered_set<int> seen{1, 2, 3};
  auto inserted = seen.insert(2);
  seen.insert(4);
  std::cout << seen.size() << ' ' << (inserted.second ? "new" : "exists")
            << '\n';
  std::cout << (seen.count(3) ? "found" : "missing") << ' '
            << (seen.count(9) ? "found" : "missing") << '\n';
  seen.erase(1);
  std::cout << seen.size() << '\n';

  std::unordered_set<Point, PointHash, PointEqual> points;
  points.insert(Point{1, 2});
  points.insert(Point{1, 2});
  points.insert(Point{3, 4});
  std::cout << points.size() << ' '
            << (points.count(Point{1, 2}) ? "yes" : "no") << '\n';
}
```

输出 `4 exists`、`found missing`、`3` 与 `2 yes`。重复插入不会改变大小，`count` 适合成员判断，自定义 `Point` 通过哈希与相等函数去重。完整版本见 [`examples/cpp11/unordered_set.cpp`](../../examples/cpp11/unordered_set.cpp)。

## 进阶用法

**预留容量。** 已知大致元素数量时先 `reserve(n)`，减少重哈希次数；`max_load_factor()` 调节桶的拥挤程度，调高省内存但冲突更多，调低则相反。

**自定义哈希与相等。** 自定义类型要么特化 `std::hash`，要么把哈希和相等函数对象作为模板参数传入；两者必须一致：相等的键必须产生相同哈希。

**透明查找。** C++20 的 `contains(key)` 比 `find(key) != end()` 更直接；当哈希器支持透明查找时，可以用 `std::string_view` 直接查 `std::string` 键，避免构造临时字符串。

**批量删除。** C++20 的 `std::erase_if(container, predicate)` 把满足条件的元素一次删掉，避免手写迭代器循环和失效处理。

**节点句柄。** C++17 的 `extract` 与 `insert(node)` 可以在不重新分配元素的情况下把节点从一个容器搬到另一个，适合需要保持元素地址稳定的场景。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                 | 用法                               | 为什么典型                                     |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------- | ---------------------------------------------- |
| drogon | [`orm_lib/src/DbClientImpl.h#L96-L102`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/src/DbClientImpl.h#L96-L102) | 连接池维护全部、空闲与忙碌连接集合 | 只关心连接是否在集合中，增删频繁，不需要顺序   |
| drogon | [`lib/inc/drogon/CacheMap.h#L55-L59`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/CacheMap.h#L55-L59)     | 用集合作为缓存桶，回调天然去重     | 同一个回调不应重复出现，集合语义直接表达唯一性 |
| mold   | [`src/mold.h#L2003-L2007`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/mold.h#L2003-L2007)                                      | 命令行选项按文件名、符号名去重     | 重复传入同一选项只保留一次，查询只判断包含关系 |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖连接管理、回调去重和选项解析三种形态。

## 陷阱与边界

**没有 `operator[]`。** `unordered_set` 只有键没有值，不能用下标访问；成员判断用 `count`、`find` 或 C++20 的 `contains`。

**元素不可变。** 集合中的元素是 `const`，不能通过迭代器修改；需要更新键时先删除旧元素再插入新元素。

**重哈希使迭代器失效。** 插入触发重哈希时迭代器全部失效，但指向元素的引用和指针仍然有效；删除只使被删元素的迭代器失效。

**哈希与相等必须一致。** 两个相等的对象必须得到相同哈希，否则容器会同时存入“相等”的重复元素；自定义键要同时检查这两个函数。

**`string_view` 键的生命周期。** 视图不拥有字符存储，底层字符串销毁后键就悬空；用于驻留表时要确保存储活得比容器久。

**遍历顺序不稳定。** 顺序取决于哈希、桶数和实现，不能依赖；需要稳定输出时先复制到 `std::vector` 再排序。

**哈希碰撞可能被利用。** 攻击者构造大量同桶键会把平均 `O(1)` 退化为 `O(n)`；处理不可信输入时应使用带随机种子的哈希或限制输入规模。

## 库联动

- **`<unordered_set>`**：容器本体；对应的 `<unordered_map>` 提供键值版本。
- **`<functional>`**：`std::hash` 与 `std::equal_to` 是默认哈希和相等策略，自定义键可特化或传入函数对象。
- **`<algorithm>`**：C++20 的 `std::erase_if` 可以直接按条件批量删除元素。
- **Abseil / TBB**：`absl::flat_hash_set` 用开放寻址降低指针追逐，`tbb::concurrent_unordered_set` 提供并发插入查找。
- **drogon / mold**：分别展示连接集合、回调桶去重与命令行选项去重三种生产用法。

## 横向对比

| 方案                  | 平均查找 | 有序遍历 | 去重 | 主要限制                       |
| --------------------- | -------- | -------- | ---- | ------------------------------ |
| `std::unordered_set`  | O(1)     | 否       | 是   | 哈希质量、碰撞攻击、顺序不稳定 |
| `std::set`            | O(log n) | 是       | 是   | 每次操作有树维护成本           |
| `std::vector`         | O(n)     | 是       | 否   | 元素多时线性查找不可接受       |
| `absl::flat_hash_set` | O(1)     | 否       | 是   | 非标准库，依赖 Abseil          |

跨标准演进：C++11 通过 N1456 引入无序关联容器；C++17 增加节点句柄与 `merge`；C++20 增加 `contains()`、透明哈希查找和 `std::erase_if`；C++23 继续补充异构查找的细节，但集合语义没有改变。
