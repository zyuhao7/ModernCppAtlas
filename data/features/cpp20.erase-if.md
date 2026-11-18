---
id: cpp20.erase-if
title: std::erase_if
standard: C++20
kind: library
category: [容器, 算法]
difficulty: 2
status: final
tier: A
compilers: {gcc: "9", clang: "8", msvc: "19.25"}
tags: [erase_if, erase, 删除元素, 容器过滤, 统一删除]
alternatives: [erase-remove 惯用法, 容器成员 erase, ranges::remove_if, swap-and-pop]
proposal: P1209R0
last_verified: "2026-11-18"
claims:
  - id: c1
    statement: "对 vector 中的 pair 列表用 erase_if 按两个字段同时匹配删除元素"
    evidence: [ev-hyprland-erase-if-lifecycle-001]
  - id: c2
    statement: "链接器用 erase_if 从 chunk 列表中剔除不占文件空间的段"
    evidence: [ev-mold-erase-if-chunks-002]
  - id: c3
    statement: "对 unordered_map 用 erase_if 按映射值删除键值对，免去 find 后 erase"
    evidence: [ev-nano-node-erase-if-lmdb-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

从标准容器里删除满足条件的元素，在 C++20 之前要分容器写：

- 序列容器用 erase-remove 惯用法，`std::remove_if` 搬移元素再调用成员 `erase`，两步容易只写一半。
- 关联容器不能用 `remove_if`，只能自己遍历，用 `it = c.erase(it)` 修正迭代器。
- 字符串有 `std::remove` 加 `erase` 的额外组合。

每类容器的删除写法不同，谓词和迭代器管理容易出错。P1209R0 把接口统一成 `std::erase_if(container, pred)`：传容器和谓词即可，函数返回删除的元素个数，迭代器与长度都由实现维护。配套的 `std::erase(container, value)` 负责按值删除。

## 基础用法

`std::erase_if` 对序列容器、关联容器和无序容器都可用。下面的例子删除 vector 中的偶数。

<!-- compile -->

```cpp
#include <iostream>
#include <vector>

int main() {
  std::vector<int> values{1, 2, 3, 4, 5, 6};
  const auto removed = std::erase_if(values, [](int value) {
    return value % 2 == 0;
  });
  std::cout << removed << ' ' << values.size() << '\n';
}
```

输出 `3 3`。完整版本见 [`examples/cpp20/erase_if.cpp`](../../examples/cpp20/erase_if.cpp)，五个场景的输出为：

```text
[1] removed = 3, left = 1 3 5
[2] removed = 1, size = 2
[3] size = 1, kept = one
[4] erased = 3, size = 2
[5] removed = 0
```

场景 1 从 vector 删除三个偶数，返回值是删除个数，剩余元素保持原顺序。场景 2 在 `std::map` 中按映射值删除分数低于 50 的条目。场景 3 在 `std::unordered_map` 中按键删除 `first > 1` 的条目。场景 4 用 `std::erase` 删除所有等于 2 的元素。场景 5 的谓词不匹配任何元素，返回 0，容器不变。

对序列容器，谓词接收元素；对关联容器，谓词接收 `value_type`，也就是键值对。返回值类型是容器的 `size_type`，可以直接与 `size()` 比较。

## 进阶用法

**组合多个字段写谓词。** Hyprland 从工作区返回列表中删除条目时，谓词同时比较工作区标识与显示器地址，只有两项都匹配才删除。如果拆成两次 `find` 再 `erase`，不仅要处理迭代器失效，还会漏掉“标识相同但地址不同”的情况。

**把过滤条件写成独立 lambda。** mold 整理输出段时用多行 lambda 检查节头类型，把 `SHT_NOBITS` 这类不占文件空间的段从 chunk 列表中剔除。谓词可读性提高，删除逻辑仍是一行调用。

**按键值对的值删除。** nano-node 关闭 LMDB 表时，`table_handles` 是 `unordered_map`，谓词只比较 `pair.second` 与待关闭的句柄。关联容器没有“按值删除”的成员函数，`erase_if` 正好补上这一操作。

**善用返回值。** 删除数量可直接用于日志、断言或决定后续是否重排；返回 0 表示容器未变，不必再比较大小。

**字符串与序列容器共用同一入口。** `std::erase_if(std::string&, pred)` 与容器的接口一致；字符串页面覆盖了字符级用法，容器页面关注元素级过滤，两者规则相同。

## 真实项目证据

| 项目                   | 位置                                                                                                                                                                                  | 用法                                            | 为什么典型                                                      |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------- | --------------------------------------------------------------- |
| hyprwm/Hyprland        | [`src/state/workspace/LifecyclePolicy.cpp#L59-L60`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/state/workspace/LifecyclePolicy.cpp#L59-L60) | 对 `vector<pair<...>>` 用多字段谓词删除条目     | 谓词组合两个字段，展示一次遍历完成复杂过滤                      |
| rui314/mold            | [`src/passes.cc#L1661-L1663`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/passes.cc#L1661-L1663)                                                 | 用多行 lambda 从 chunk 列表剔除不占文件空间的段 | 链接器热路径需要就地过滤，避免手写迭代器删除                    |
| nanocurrency/nano-node | [`nano/store/lmdb/backend_lmdb.cpp#L196-L198`](https://github.com/nanocurrency/nano-node/blob/6b5f63ce04c9a1ffe37b8c687fffc1bef941bf98/nano/store/lmdb/backend_lmdb.cpp#L196-L198)    | 对 `unordered_map` 按映射值删除句柄记录         | 关联容器没有按值删除成员，`erase_if` 免去显式 `find` 再 `erase` |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径，覆盖序列容器、链接器输出整理与无序关联容器三种场景。

## 陷阱与边界

**谓词内不要修改容器。** 删除过程中修改容器会导致迭代器失效或未定义行为；需要联动修改时，先收集结果，删除结束后再处理。

**关联容器的元素是键值对。** `std::map`、`std::unordered_map` 的谓词参数是 `pair<const Key, T>`，按值或按 `const&` 接收都可以，但不能通过它修改键。

**迭代器与引用会失效。** 删除发生后，指向被删元素及其后位置的迭代器、引用、指针都不再有效；需要保留的元素应先复制或移动出去。

**序列容器保持相对顺序。** `erase_if` 等价于 `remove_if` 加 `erase`，剩余元素按原顺序排列；如果顺序不重要且追求更低搬移成本，swap-and-pop 可能更快，但要自己写。

**返回的是删除个数。** 类型为 `size_type`，不是布尔值；判断“是否发生删除”要与 0 比较。

**与成员 `erase` 的分工。** `std::erase_if` 是自由函数，按谓词删除；成员 `erase` 按键或迭代器删除。对关联容器按值过滤用前者，按已知键删除仍用成员 `erase`。

**不是范围算法。** `std::erase_if` 不接受迭代器区间，也不支持 `std::span` 或裸数组；对视图只能先复制到容器再过滤。

**支持时间。** GCC 9、Clang 8、MSVC 19.25 起提供；特性测试宏为 `__cpp_lib_erase_if >= 202002L`。

## 库联动

- **`std::erase`**：C++20 的按值删除版本，与 `erase_if` 一起构成统一删除接口。
- **`std::remove_if`**：算法层的搬移版本，只重排不删除；`erase_if` 内部通常就是它加成员 `erase`。
- **容器成员 `erase`**：按迭代器或键删除的底层操作，`erase_if` 的返回值和失效规则都建立在它之上。
- **`std::ranges::remove_if`**：范围版本同样只重排；要真正缩短容器仍需配合 `erase` 或使用 `erase_if`。
- **`std::basic_string`**：C++20 也提供 `std::erase` 与 `std::erase_if`，字符级过滤与容器接口一致。

## 横向对比

| 方案                | 适用容器             | 迭代器管理       | 返回值       | 主要限制                               |
| ------------------- | -------------------- | ---------------- | ------------ | -------------------------------------- |
| `std::erase_if`     | 序列、关联、无序容器 | 实现负责         | 删除个数     | C++20 起；谓词不能修改容器             |
| erase-remove 惯用法 | 序列容器             | 调用方负责       | 无           | 两步容易漏写 `erase`，不适用于关联容器 |
| 成员 `erase` 循环   | 所有容器             | 调用方修正迭代器 | 迭代器或个数 | 手写循环，删除条件分散                 |
| `ranges::remove_if` | 范围                 | 不删除           | 子范围       | 只重排，容器长度不变                   |
| swap-and-pop        | 序列容器             | 调用方负责       | 无           | 打乱顺序，适合顺序无关的场景           |

`std::erase_if` 由 P1209R0 进入 C++20，把不同容器的删除写法统一成一个自由函数。它不改变各容器的复杂度，也不替代按已知键删除的成员 `erase`，但把谓词过滤从“记住每类容器的惯用法”变成一次调用。
