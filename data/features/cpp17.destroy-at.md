---
id: cpp17.destroy-at
title: std::destroy_at
standard: C++17
kind: library
category: [对象模型, 内存管理]
difficulty: 3
status: final
tier: A
compilers: {gcc: "7.1", clang: "4", msvc: "19.11"}
tags: [destroy_at, 析构, 对象生命周期, 原始存储]
alternatives: [显式析构, std::destroy, allocator_traits::destroy, RAII]
proposal: P0040R3
last_verified: "2026-11-20"
claims:
  - id: c1
    statement: "结束手动生命周期容器中当前对象的生命周期"
    evidence: [ev-stdexec-destroy-at-manual-lifetime-001]
  - id: c2
    statement: "按活动索引只销毁 variant 当前持有的类型"
    evidence: [ev-stdexec-destroy-at-variant-002]
  - id: c3
    statement: "逐个析构小型映射中已构造的元素"
    evidence: [ev-glaze-destroy-at-small-map-003]
review:
  author: "Codex"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

容器可能一次分配多个槽位，却只在其中构造部分元素。删除元素或回滚失败的构造时，容器要调用活对象的析构函数，同时保留存储供后续使用。C++17 的 `std::destroy_at(p)` 把单个对象的显式析构封装成可从指针推导类型的函数。

例如一个容器有 8 个槽位，却只存了 3 个字符串。清空时要析构这 3 个字符串，保留容量便于下一次插入。普通成员和局部变量继续用自动析构，只有自己管理存储和对象生命周期时才需要这个工具。

## 基础用法

以下示例先在对齐的字节缓冲区构造字符串。打印后，`destroy_at` 调用字符串的析构函数，字节数组仍可以复用。

<!-- compile -->

```cpp
#include <cstddef>
#include <iostream>
#include <memory>
#include <new>
#include <string>

int main() {
  alignas(std::string) std::byte storage[sizeof(std::string)];
  auto* text = new (storage) std::string("ready");
  std::cout << *text << '\n';
  std::destroy_at(text);
}
```

最小示例输出 `ready`。完整示例用计数器观察四种场景下的析构次数。[destroy_at.cpp](../../examples/cpp17/destroy_at.cpp)

```text
[1] first destruction = 1
[2] after slot reuse = 2
[3] after destroying two live slots = 4
[4] after scope exit = 5
```

场景 1 在字节缓冲区构造一个对象，调用 `destroy_at` 后计数变为 1。场景 2 复用同一槽位，通过新构造返回的指针再次析构，计数变为 2。

场景 3 从分配器申请三个槽位，只构造前两个。`destroy_n(slots, 2)` 析构这两个对象，随后用原容量 3 释放存储。场景 4 让带自定义删除器的 `unique_ptr` 在离开作用域时调用 `destroy_at`，计数最终为 5。

## 进阶用法

**记录当前活对象。** 维护多种可能类型的槽位时，用活动索引决定析构哪一种。stdexec 的 variant 实现先取出索引并改为空状态，再通过索引选中相应的 `destroy_at` 调用。固定元素类型的容器只需记录已经构造的数量。

**用已构造数量回滚。** 一次分配三个槽位，若第三次构造抛出异常，清理时只析构前两个，然后释放三个槽位的存储。每次构造成功后再增加计数。若算法需要特定的逆序清理，可用显式倒序循环逐个调用 `destroy_at`，因为 `destroy_n` 从首元素向后处理。

**把析构绑定到作用域。** 示例中的删除器只结束对象生命周期，外层字节数组继续持有存储。这种写法适用于单个手动构造的对象，能让提前返回也执行析构。整段存储的分配和回收仍由分配器或容器负责。

## 真实项目证据

| 项目               | 固定源码                                                                                                                                                                                                   | 用法                                     | 为什么典型                                                 |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------- | ---------------------------------------------------------- |
| NVIDIA/stdexec     | [`include/stdexec/__detail/__manual_lifetime.hpp#L67-L72`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__manual_lifetime.hpp#L67-L72)         | 结束手动生命周期容器中当前对象的生命周期 | 存储归容器持有，结束元素生命周期时无需释放底层缓冲区。     |
| NVIDIA/stdexec     | [`include/stdexec/__detail/__variant.hpp#L153-L161`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__variant.hpp#L153-L161)                     | 按活动索引只销毁 variant 当前持有的类型  | 索引决定存储中哪一种对象处于存活状态，空状态无需析构。     |
| stephenberry/glaze | [`include/glaze/containers/ordered_small_map.hpp#L151-L156`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/containers/ordered_small_map.hpp#L151-L156) | 逐个析构小型映射中已构造的元素           | 循环按实际构造数量结束元素生命周期，容量中的空槽位不参与。 |

三条证据均来自固定 commit 的生产路径，现为 `manual` 级。

## 陷阱与边界

- 指针必须指向要销毁的活对象。对空指针、尚未构造的槽位或已析构的对象再次调用都会违反前置条件。
- 用非平凡析构类型声明普通局部变量后，作用域退出时仍会自动调用其析构函数。若提前销毁后没有在原处及时构造同类型对象，作用域退出时再次析构会产生未定义行为。
- 在存储上重新构造后，优先使用构造表达式返回的指针。析构本身不释放对象所占的存储。如果存储来自分配器，全部对象析构后再按对应的分配方式释放。
- C++17 的 `destroy_at` 用于非数组对象。C++20 新增对数组递归析构的支持，也允许在满足常量求值约束时调用它。

## 库联动

- [std::construct_at](cpp20.construct-at.md) 是 C++20 中对应的单对象构造工具。C++17 示例用 placement new。
- 使用自定义分配器的容器可以通过 `std::allocator_traits<A>::destroy` 保留分配器的析构定制。
- [std::optional](cpp17.optional.md) 的 `reset()` 适合只需持有零个或一个对象的业务代码。它会管理是否已经构造以及何时析构。

## 横向对比

| 写法                        | 适用场景                                   |
| --------------------------- | ------------------------------------------ |
| `std::destroy_at(p)`        | 已持有一个活对象的地址，并打算保留底层存储 |
| `std::destroy(first, last)` | 析构一段连续或可迭代的活对象序列           |
| `std::destroy_n(first, n)`  | 已知从起点起有多少个活对象                 |
| 自动析构                    | 让作用域或所有者负责对象生命周期           |

标准依据见 [P0040R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0040r3.html)。
