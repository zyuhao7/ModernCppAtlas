---
id: cpp11.shared-ptr
title: std::shared_ptr 与 std::weak_ptr
standard: C++11
kind: library
category: [所有权, 内存管理]
difficulty: 3
status: final
tier: B
compilers: {gcc: "4.4", clang: "3.0", msvc: "16.0"}
tags: [shared_ptr, weak_ptr, 引用计数, make_shared, 循环引用]
alternatives: [unique_ptr, 值语义, intrusive_ptr, 裸指针观察]
proposal: N2351
last_verified: "2026-10-30"
claims:
  - id: c1
    statement: "用 make_shared 创建处理器并让异步队列共享其生命周期"
    evidence: [ev-drogon-shared-ptr-binder-001]
  - id: c2
    statement: "用 shared_ptr 返回注册表中的 logger，共享同一实例"
    evidence: [ev-spdlog-shared-ptr-registry-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

有些对象天然被多个所有者共享：连接、缓存项、事件回调可能跨越多个线程或异步任务。用裸指针共享时要手工约定“谁最后释放”，引用计数稍微写错就会提前析构或泄漏。`unique_ptr` 又只能有一个所有者，复制会被编译器拒绝。

`std::shared_ptr` 用控制块保存引用计数，最后一个所有者析构时释放对象；`std::weak_ptr` 观察共享对象但不增加计数，用来打破循环引用或在访问前检查对象是否还活着。标准提案为 N2351。

## 基础用法

`std::make_shared` 一次分配对象与控制块；`use_count()` 查看当前强引用数；`weak_ptr::lock()` 在对象存活时取得 `shared_ptr`。

<!-- compile -->

```cpp
#include <iostream>
#include <memory>

struct Node {
  std::shared_ptr<Node> next;
  std::weak_ptr<Node> parent;
  int value = 0;
};

int main() {
  auto first = std::make_shared<Node>();
  auto second = std::make_shared<Node>();
  first->value = 1;
  second->value = 2;
  first->next = second;
  second->parent = first;

  std::cout << first.use_count() << ' ' << second.use_count() << '\n';
  if (auto locked = second->parent.lock()) {
    std::cout << locked->value << '\n';
  }
}
```

输出 `1 2` 与 `1`。`parent` 用 `weak_ptr` 保存，因此 `first` 的计数仍为 1；若改成 `shared_ptr`，两个节点会互相持有，计数永远不归零。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                 | 用法                                      | 为什么典型                                 |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------- | ------------------------------------------ |
| drogon | [`lib/inc/drogon/HttpAppFramework.h#L542-L545`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/HttpAppFramework.h#L542-L545) | `make_shared` 创建处理器，lambda 按值捕获 | 异步任务与注册表共享对象，引用计数延长寿命 |
| spdlog | [`include/spdlog/details/registry-inl.h#L86-L90`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/registry-inl.h#L86-L90)      | 注册表按名字返回 `shared_ptr<logger>`     | 多个调用点共享同一 logger，复制句柄即可    |

## 陷阱与边界

**循环引用会泄漏。** 两个对象互相持有 `shared_ptr` 时，计数无法归零。父子、前后向关系至少有一侧改用 `weak_ptr`，并在访问前 `lock()`。

**`weak_ptr` 不能直接解引用。** 必须先 `lock()` 得到 `shared_ptr`，检查非空后再用；直接取出裸指针可能在另一线程释放后悬空。

**`make_shared` 与 `shared_ptr(new T)` 的差异。** 前者一次分配对象和控制块，通常更快；但对象与控制块同生命周期，若还有 `weak_ptr`，内存要等弱引用也消失才释放。

**不要用同一裸指针构造多个 `shared_ptr`。** 每个控制块独立计数，最终会二次释放。需要从 `this` 获得共享句柄时用 `enable_shared_from_this`。

**引用计数是原子的，对象不是。** 多个线程可以安全地复制、销毁 `shared_ptr`，但同时对同一对象成员读写仍需要自己的同步。

**别名构造不是类型转换。** `shared_ptr<T>(other, ptr)` 共享 `other` 的控制块但指向 `ptr`，用于指向成员或子对象；它不会延长 `ptr` 的独立生命周期。

## 横向对比

| 方案                | 所有权 | 复制成本     | 循环引用风险 | 主要限制                   |
| ------------------- | ------ | ------------ | ------------ | -------------------------- |
| `unique_ptr`        | 独占   | 禁止复制     | 无           | 无法共享                   |
| `shared_ptr`        | 共享   | 原子计数增减 | 有           | 控制块与原子操作成本       |
| `weak_ptr`          | 不拥有 | 原子弱计数   | 无           | 使用前必须 `lock()`        |
| 裸指针观察          | 不拥有 | 无           | 无           | 无法判断对象是否仍然存活   |
| 值语义 / `optional` | 独占   | 值复制或移动 | 无           | 大对象复制昂贵，不适合多态 |

跨标准演进：C++11 引入 `shared_ptr`、`weak_ptr`、`make_shared` 与 `enable_shared_from_this`；C++17 增加 `weak_from_this`；C++20 起 `make_shared` 支持数组类型。
