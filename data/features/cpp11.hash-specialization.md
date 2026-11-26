---
id: cpp11.hash-specialization
title: std::hash 特化（自定义类型作键）
standard: C++11
kind: library
category: [容器, 类型工具]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.1", msvc: "16.0"}
tags: [std::hash, 哈希特化, unordered_set, 自定义键, operator==]
alternatives: [自定义 Hash 模板参数, absl::Hash, std::map, 预计算哈希]
proposal: N1456
last_verified: "2026-11-25"
claims:
  - id: c1
    statement: "为自定义 uuid 类型特化 std::hash，使其能作为无序容器键"
    evidence: [ev-glaze-hash-uuid-001]
  - id: c2
    statement: "在特化里保存随机化密钥，提供抗碰撞的哈希"
    evidence: [ev-nghttp2-hash-quic-cid-002]
  - id: c3
    statement: "复用成员类型的 std::hash 结果组合出复合键的哈希"
    evidence: [ev-vroom-hash-location-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

`std::unordered_map` 与 `std::unordered_set` 默认用 `std::hash<Key>` 计算哈希。标准库只为内置类型和少数标准类型提供了特化；把自定义结构体直接当键时，缺少哈希函数会导致编译失败。

C++11 的解决办法是为自定义类型特化 `std::hash`。特化一旦存在，所有默认使用 `std::hash` 的容器与算法都会自动采用它，调用方不必在每处显式传哈希对象。特化必须与类型的 `operator==` 语义一致：相等的对象必须产生相同的哈希值。

## 基础用法

特化写在 `namespace std` 内，提供一个 `constexpr` 可调用对象（通常就是 `operator()`），接收键类型并返回 `size_t`。键类型本身还需要 `operator==`。

<!-- compile -->

```cpp
#include <cstddef>
#include <iostream>
#include <unordered_set>

struct Point {
  int x;
  int y;

  bool operator==(const Point& other) const {
    return x == other.x && y == other.y;
  }
};

namespace std {
template <>
struct hash<Point> {
  size_t operator()(const Point& p) const noexcept {
    return hash<int>()(p.x) * 31u + hash<int>()(p.y);
  }
};
}  // namespace std

int main() {
  std::unordered_set<Point> points;
  points.insert(Point{1, 2});
  points.insert(Point{1, 2});
  points.insert(Point{3, 4});
  std::cout << points.size() << ' ' << points.count(Point{3, 4}) << '\n';
}
```

完整示例见 [`examples/cpp11/hash_specialization.cpp`](../../examples/cpp11/hash_specialization.cpp)，输出为：

```text
[1] hash(Point{1, 2}) stable = 1
[2] unordered_set size after 3 inserts = 2
[3] contains(Point{3, 4}) = 1, contains(Point{5, 6}) = 0
```

场景 1 验证相同输入得到相同哈希；场景 2 插入三个元素，其中两个相等，集合只保留两个；场景 3 检查成员查询，命中的坐标返回 1，未命中的返回 0。

## 进阶用法

**组合已有类型的哈希。** 复合键可以先对每个成员调用标准特化，再把结果混合。简单的 `hash<int>()(x) * 31 + hash<int>()(y)` 足够应付教学与小规模数据；生产环境建议使用成熟的混合函数，避免低位规律导致冲突。

**有状态的哈希对象。** 特化不一定无状态。`nghttp2` 在 `std::hash<ngtcp2_cid>` 里保存 SipHash 密钥，用随机化抵抗哈希碰撞攻击；容器按值保存哈希对象，构造时生成密钥。

**用模板参数替代全局特化。** 不想触碰 `namespace std` 时，可以把哈希写成独立函数对象，作为 `unordered_map` 的第二个模板参数传入。特化的优势是“默认生效”，缺点是影响全局。

**保证与 `operator==` 一致。** 哈希只决定桶位置，相等判断仍由 `operator==` 完成；两者对“相等”的定义必须一致，否则会出现“相同键查不到”的隐蔽缺陷。

## 真实项目证据

| 项目                | 固定源码                                                                                                                                                                  | 用法                                      | 为什么典型                                     |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------- | ---------------------------------------------- |
| stephenberry/glaze  | [`include/glaze/util/uuid.hpp#L105-L113`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/util/uuid.hpp#L105-L113)      | 为 `glz::uuid` 特化 `std::hash`           | uuid 是值类型，需要直接作为无序容器键          |
| nghttp2/nghttp2     | [`src/shrpx_quic.h#L60-L72`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/shrpx_quic.h#L60-L72)                                   | 特化里保存 SipHash 密钥                   | 面向网络的键需要随机化哈希，抵抗碰撞攻击       |
| VROOM-Project/vroom | [`src/structures/vroom/location.h#L59-L72`](https://github.com/VROOM-Project/vroom/blob/43dd7d0b8b560431eb555bf335cf4797eb7343c4/src/structures/vroom/location.h#L59-L72) | 复用成员类型的 `std::hash` 组合复合键哈希 | 复合键的哈希可以逐成员计算，语义与相等判断对齐 |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **只对自定义类型特化。** 标准禁止为用户没有定义的类型（尤其是标准库类型）添加 `std::hash` 特化；需要自定义标准类型的哈希时，应传入独立的哈希函数对象。
- **必须与 `operator==` 一致。** 若 `a == b` 为真但哈希不同，容器会认为它们是不同的键；反过来，哈希相同但对象不等只是冲突，不破坏正确性。
- **`noexcept` 很关键。** 哈希函数抛出异常会让容器处于不确定状态；实践里几乎总是标记 `noexcept`，并避免分配内存。
- **哈希质量影响性能。** 简单拼接容易让低位呈现规律，导致桶分布不均；对性能敏感的场景要用混合函数或成熟哈希库。
- **不要返回不稳定值。** 用随机数或指针地址做哈希会破坏同一次运行内的查找；有状态哈希也必须在对象生命周期内保持稳定。
- **特化要可见。** 特化必须在使用容器的所有翻译单元中可见，通常与键类型定义放在同一头文件。

## 库联动

- [std::unordered_map](cpp11.unordered-map.md) 与 [std::unordered_set](cpp11.unordered-set.md)：默认使用 `std::hash` 的无序容器。
- **`std::hash` 标准特化**：`std::string`、`std::shared_ptr`、`std::filesystem::path` 等类型已有标准提供或标准库实现的哈希。
- **`absl::Hash`**：Abseil 提供的通用哈希框架，支持自动组合成员，避免手写特化。
- **`std::equal_to`**：容器的相等谓词，必须与哈希语义一致。
- **自定义哈希模板参数**：`unordered_map<Key, Value, MyHash>`，不想特化 `std::hash` 时的替代方案。

## 横向对比

| 方案               | 生效范围     | 可读性 | 主要限制                     |
| ------------------ | ------------ | ------ | ---------------------------- |
| `std::hash` 特化   | 全局默认     | 高     | 只能用于自定义类型           |
| 自定义哈希模板参数 | 单个容器实例 | 中     | 每处使用都要显式传入         |
| `absl::Hash`       | 显式调用     | 高     | 引入 Abseil 依赖             |
| `std::map`         | 有序容器     | 高     | 需要严格弱序，查找为对数时间 |
| 预计算哈希         | 数据成员     | 中     | 增加对象大小，需手动维护     |

`std::hash` 特化由 N1456 随无序容器一起进入 C++11，是让自定义值类型直接作为哈希键的最短路径。它适合稳定的值类型与库内公共类型；若只是某个容器需要特殊哈希，或用不了全局特化，传给容器的自定义哈希对象更合适。
