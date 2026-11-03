---
id: cpp11.unordered-map
title: std::unordered_map
standard: C++11
kind: library
category: [容器, 性能]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.1", msvc: "16.0"}
tags: [std::unordered_map, 哈希表, 键值容器, 均摊常数时间, 哈希]
alternatives: [std::map, absl::flat_hash_map, std::vector 线性查找, 有序数组]
proposal: N1456
last_verified: "2026-11-03"
claims:
  - id: c1
    statement: "用 unordered_map 按名称索引 logger"
    evidence: [ev-spdlog-unordered-map-registry-001]
  - id: c2
    statement: "用 unordered_map 建立字符串视图到偏移量的索引"
    evidence: [ev-mold-unordered-map-strings-002]
  - id: c3
    statement: "用 unordered_map 作为缓存主存储"
    evidence: [ev-drogon-unordered-map-cache-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

需要按键快速取回值，但不关心遍历顺序时，`std::map` 的红黑树给出 `O(log n)` 查找并维护排序，代价是每次操作都要比较、旋转和额外节点分配。数据量一大，对数因子和指针追逐就成为热点。

`std::unordered_map<Key, T>` 用哈希表提供平均 `O(1)` 的查找、插入和删除，不保证元素顺序。它适合“按名字取对象”“计数器”“缓存”这类只关心键值映射的场景。标准提案为 N1456。

## 基础用法

`operator[]` 在键不存在时插入值初始化的元素，因此读取会意外扩容；只读访问用 `at()`，查找用 `find()` 或 `count()`。`emplace()` 返回 `pair<iterator, bool>`，`second` 表示是否真正插入。

<!-- compile -->

```cpp
#include <iostream>
#include <string>
#include <unordered_map>

int main() {
  std::unordered_map<std::string, int> counts;
  counts["apple"] = 2;
  counts["pear"] += 1;

  auto result = counts.emplace("apple", 5);
  std::cout << counts.size() << ' ' << counts.at("apple") << ' '
            << (result.second ? "new" : "exists") << '\n';
  std::cout << (counts.find("pear") != counts.end() ? "found" : "missing")
            << '\n';
}
```

输出 `2 2 exists` 与 `found`。`emplace("apple", 5)` 没有覆盖已有值，`second` 为 `false`。完整版本见 [`examples/cpp11/unordered_map.cpp`](../../examples/cpp11/unordered_map.cpp)。

## 进阶用法

**预留容量。** 已知元素数量时先 `reserve(n)`，把重哈希次数降到最低；`max_load_factor()` 控制桶的拥挤程度，调高节省内存但增加冲突，调低则相反。

**避免不必要的临时对象。** C++17 的 `try_emplace(key, args...)` 只在键不存在时构造值，`insert_or_assign` 则显式表达“插入或覆盖”。C++17 还提供节点句柄，可以在不重新分配的情况下把元素从一个容器搬到另一个容器。

**C++20 增加 `contains()` 与异构查找。** `contains(key)` 比 `find() != end()` 更直接；当哈希器支持透明查找时，可以用 `std::string_view` 直接查 `std::string` 键，避免临时字符串。

**自定义哈希与相等。** 自定义类型作为键时，提供 `std::hash` 特化或传入哈希/相等函数对象。哈希质量直接决定冲突率；对不可信输入，还要考虑哈希碰撞攻击，标准库实现通常不保证抗碰撞。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                        | 用法                                  | 为什么典型                                                   |
| ------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------- | ------------------------------------------------------------ |
| spdlog | [`include/spdlog/details/registry.h#L111-L113`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/registry.h#L111-L113) | 按名称索引 `shared_ptr<logger>`       | 注册表只需按名字取回对象，不需要有序遍历，哈希查找是自然选择 |
| mold   | [`src/mold.h#L731-L733`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/mold.h#L731-L733)                                                 | 用 `string_view` 键建立字符串驻留索引 | 键指向已有稳定存储，避免复制字符串，同时保持均摊常数时间查找 |
| drogon | [`lib/inc/drogon/CacheMap.h#L450-L450`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/CacheMap.h#L450-L450)        | 用模板 `unordered_map` 作为缓存主存储 | 缓存只按键读写、不要求顺序，过期清理由独立时间轮承担         |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖注册表、字符串驻留和缓存三种形态。

## 陷阱与边界

**`operator[]` 会插入。** 读取不存在的键会插入默认值并可能触发重哈希；只读场景用 `at()`（越界抛 `std::out_of_range`）或 `find()`。

**重哈希会使迭代器失效。** 插入导致重哈希时，所有迭代器失效，但指向元素的引用和指针仍然有效；删除只使被删元素的迭代器失效。

**遍历顺序不稳定。** 顺序取决于哈希、桶数和实现，不能依赖；需要稳定输出时先复制到 `std::vector` 再排序。

**键必须可哈希且不可变。** 键在容器内是 `const`，不能通过迭代器修改；自定义键要保证相等与哈希一致。

**`std::string_view` 键的生命周期。** 视图不拥有字符存储，底层字符串销毁后键就悬空；用于驻留表时要确保存储活得比容器久。

**哈希碰撞可能被利用。** 攻击者构造大量同桶键会把平均 `O(1)` 退化为 `O(n)`；处理不可信输入时应使用带随机种子的哈希或限制输入。

## 库联动

- **`<unordered_map>`**：容器本体；配套的 `<unordered_set>` 提供集合版本。
- **`<functional>`**：`std::hash`、`std::equal_to` 是默认哈希与相等策略，自定义键可特化或传入函数对象。
- **`<string_view>`**：C++17 起常用作只读键，前提是底层存储生命周期足够长。
- **abseil / TBB**：`absl::flat_hash_map` 用开放寻址降低指针追逐，`tbb::concurrent_unordered_map` 提供并发插入查找。
- **spdlog / mold / drogon**：分别展示名称注册表、字符串驻留表与缓存主存储三种生产用法。

## 横向对比

| 方案                   | 平均查找 | 有序遍历 | 迭代器稳定性       | 主要限制                       |
| ---------------------- | -------- | -------- | ------------------ | ------------------------------ |
| `std::unordered_map`   | O(1)     | 否       | 重哈希使迭代器失效 | 哈希质量与碰撞攻击，顺序不稳定 |
| `std::map`             | O(log n) | 是       | 插入删除不影响其他 | 每次操作有树维护成本           |
| `absl::flat_hash_map`  | O(1)     | 否       | 重哈希使引用失效   | 非标准库，依赖 Abseil          |
| `std::vector` 线性查找 | O(n)     | 是       | 插入可能使全部失效 | 元素少时更快，元素多时不可接受 |

跨标准演进：C++11 通过 N1456 引入无序关联容器；C++17 增加节点句柄、`try_emplace` 与 `insert_or_assign`；C++20 增加 `contains()` 与透明哈希查找。
