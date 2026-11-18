---
id: cpp17.map-try-emplace
title: std::map::try_emplace 与 insert_or_assign
standard: C++17
kind: library
category: [容器]
difficulty: 2
status: final
tier: A
compilers: {gcc: "6.1", clang: "3.7", msvc: "19.0"}
tags: [try_emplace, insert_or_assign, map, unordered_map, 插入语义]
alternatives: [find 加 insert, "operator[]", emplace, insert 加提示位置]
proposal: N4279
last_verified: "2026-11-18"
claims:
  - id: c1
    statement: "建立映射时用 try_emplace，键已存在则跳过映射值的构造"
    evidence: [ev-vroom-try-emplace-location-001]
  - id: c2
    statement: "用 try_emplace 的布尔结果判断名称是否重复注册"
    evidence: [ev-hyprland-try-emplace-eventbus-002]
  - id: c3
    statement: "取或创建对象后递归合并，try_emplace 返回迭代器避免第二次查找"
    evidence: [ev-glaze-try-emplace-patch-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

向 `std::map` 插入元素时，`operator[]` 会先默认构造映射值再允许赋值，键是否存在都要付出一次构造；`insert` 在键重复时不插入，却仍可能先构造出临时对象；`find` 加 `insert` 要查找两次。对映射值构造昂贵、或者值类型不可默认构造的容器，这些写法要么浪费构造，要么根本无法编译。

C++17 引入 `try_emplace` 与 `insert_or_assign`：前者只在键不存在时用参数原地构造映射值，键存在时完全不碰映射值；后者在键存在时赋值、不存在时构造。两者都返回 `pair<iterator, bool>`，布尔位表示是否发生了插入，调用方一次操作就能同时拿到元素位置和结果。

## 基础用法

`try_emplace(key, args...)` 把 `args` 转发给映射值的构造函数，仅在插入发生时进行。下面的例子连续插入同一个键两次，第二次不会覆盖已有值。

<!-- compile -->

```cpp
#include <iostream>
#include <map>
#include <string>

int main() {
  std::map<std::string, std::string> entries;
  auto [it, inserted] = entries.try_emplace("key", "first");
  auto [again, inserted_again] = entries.try_emplace("key", "second");
  std::cout << inserted << ' ' << inserted_again << ' ' << it->second << '\n';
}
```

输出 `1 0 first`。完整版本见 [`examples/cpp17/map_try_emplace.cpp`](../../examples/cpp17/map_try_emplace.cpp)，六个场景的输出为：

```text
[1] inserted = 1, exists = 0
[2] value = first
[3] constructions = 1
[4] after assign = third
[5] constructions = 2
[6] size = 1, value = one
```

场景 1 中第一次插入成功、第二次因键已存在而失败。场景 2 读取保留下来的第一个值。场景 3 的计数器证明映射值只构造了一次。场景 4 用 `insert_or_assign` 覆盖为 `third`。场景 5 的计数器加一，说明覆盖确实构造了新值。场景 6 在 `unordered_map` 上重复键同样只保留第一次插入的值。

`insert_or_assign` 与 `try_emplace` 的差别在键已存在时：前者赋值覆盖，后者保持原值不变。

## 进阶用法

**去重映射一次插入。** VROOM 在建立“位置到索引”的映射时直接 `try_emplace(job.location, new_index)`：位置首次出现就记录索引，重复出现则什么都不做。位置对象作为键比较开销不小，避免第二次查找能直接省掉一次树遍历。

**用布尔位表达重复注册。** Hyprland 的事件总线注册插件事件时，把 `try_emplace(...).second` 取反作为错误条件，键重复就返回 `unexpected`。`try_emplace` 已经完成了查找与插入，调用方不必再写 `find` 判断。

**取得或创建对象。** glaze 合并 JSON patch 时先 `try_emplace(key, GenericType{})` 取得或创建目标对象，再把返回的迭代器交给递归合并。注释点明这是为了避免双重查找：键存在时复用，不存在时创建，之后只用迭代器。

**原地构造不可默认构造的值。** `try_emplace` 把参数直接传给映射值构造函数，映射值不必可默认构造、也不必可移动，适合持有 `mutex`、引用包装或带显式构造函数的类型。

**异构查找配合透明比较器。** 在 `std::map<std::string, T, std::less<>>` 上用 `std::string_view` 查找时，C++17 的异构重载可以避免为查找临时构造 `std::string`；`try_emplace` 与这些重载属于同一批接口改进。

## 真实项目证据

| 项目                | 位置                                                                                                                                                                                    | 用法                                             | 为什么典型                                       |
| ------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ | ------------------------------------------------ |
| VROOM-Project/vroom | [`src/structures/vroom/input/input.cpp#L151-L156`](https://github.com/VROOM-Project/vroom/blob/43dd7d0b8b560431eb555bf335cf4797eb7343c4/src/structures/vroom/input/input.cpp#L151-L156) | 位置去重映射用 `try_emplace`，键存在时跳过值构造 | 键比较开销大、重复频繁，正是不该二次查找的场景   |
| hyprwm/Hyprland     | [`src/event/EventBus.cpp#L23-L26`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/event/EventBus.cpp#L23-L26)                                     | 用返回的布尔位判断事件名是否重复注册             | 一次调用同时完成查重与插入，省去 `find` 分支     |
| stephenberry/glaze  | [`include/glaze/json/patch.hpp#L844-L848`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/json/patch.hpp#L844-L848)                  | `try_emplace` 取得或创建对象，迭代器交给递归合并 | 注释明示“避免双重查找”，是接口设计意图的直接体现 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径，覆盖去重建映射、注册查重与递归合并三种用法。

## 陷阱与边界

**`try_emplace` 不覆盖已有值。** 键存在时直接返回原元素，传入的参数只被求值、不参与构造；需要更新时用 `insert_or_assign` 或先取迭代器再赋值。

**参数先求值，映射值后构造。** 即使键已存在，传给 `try_emplace` 的实参表达式仍会被求值，只是不构造映射值；实参有副作用时仍会发生。

**花括号不能直接推导。** `try_emplace(key, {1, 2})` 无法推导 `Args`，需要显式写出类型，例如 `try_emplace(key, std::pair<int, int>{1, 2})`；`insert` 接受花括号是因为它另有重载。

**`insert_or_assign` 要求可赋值。** 映射值类型必须支持赋值；只可构造不可赋值的类型（如持有引用或 `const` 成员）应继续用 `try_emplace`。

**返回的迭代器可能失效。** `std::map` 插入不使其他迭代器失效，`std::unordered_map` 的 rehash 会让迭代器失效；跨插入保存迭代器时要留意容器种类。

**与 `operator[]` 的构造差异。** `operator[]` 在键不存在时默认构造映射值再赋值，要求可默认构造；`try_emplace` 没有这个要求，构造开销也更低。

**支持时间。** GCC 6.1、Clang 3.7、MSVC 19.0 起提供；两个接口都随 C++17 进入标准，没有单独的特性测试宏。

## 库联动

- **`std::map` / `std::unordered_map`**：`try_emplace` 与 `insert_or_assign` 是两者的成员函数，语义一致，失效规则随容器不同。
- **`std::pair`**：两个接口都返回 `pair<iterator, bool>`，结构化绑定可直接拆出迭代器与插入结果。
- **`std::piecewise_construct`**：`emplace` 用它分别构造键和值；`try_emplace` 已把键单独取出，通常不再需要。
- **`std::initializer_list`**：`insert` 的列表重载；`try_emplace` 不接受花括号推导，需要显式类型。
- **`std::expected`**：Hyprland 用 `try_emplace` 的布尔位决定是否返回错误，把注册失败转成 `std::unexpected`。

## 横向对比

| 方案               | 键存在时行为   | 查找次数 | 映射值构造要求       | 主要限制                   |
| ------------------ | -------------- | -------- | -------------------- | -------------------------- |
| `try_emplace`      | 保持原值不变   | 1        | 只需可用传入参数构造 | 不覆盖；花括号不能推导     |
| `insert_or_assign` | 赋值覆盖       | 1        | 需要可赋值           | 覆盖时构造新值             |
| `operator[]`       | 返回引用可赋值 | 1        | 需要可默认构造       | 不存在时先默认构造再赋值   |
| `find` 加 `insert` | 保持原值不变   | 2        | 同 `insert`          | 两次查找，键比较开销翻倍   |
| `emplace`          | 保持原值不变   | 1        | 需能就地构造键和值   | 键存在时仍可能构造临时对象 |

`try_emplace` 与 `insert_or_assign` 由 N4279 进入 C++17，把“先查找再决定插入或赋值”的常见模式收进成员函数。它们不改变关联容器的复杂度，但把重复查找和多余构造从调用方移进了实现。
