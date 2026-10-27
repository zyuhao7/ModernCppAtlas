---
id: cpp17.guaranteed-copy-elision
title: 保证的复制消除
standard: C++17
kind: language
category: [对象模型, 性能]
difficulty: 4
status: final
tier: B
compilers: {gcc: "7", clang: "4", msvc: "19.13"}
tags: [copy elision, RVO, NRVO, prvalue, 不可移动类型]
alternatives: [显式 std::move, placement new 与手动生命周期, 输出参数, 指针返回]
proposal: P0135R1
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "在 requires 表达式中用 prvalue 直接初始化不可移动类型，把复制消除当作可检测的约束"
    evidence: [ev-stdexec-copy-elision-trait-001]
  - id: c2
    statement: "成员以函数返回值直接初始化本应触发强制复制消除，GCC 未实现时退回手动生命周期管理"
    evidence: [ev-stdexec-copy-elision-member-awaitable-002]
  - id: c3
    statement: "用 placement new 在未初始化存储上直接构造，作为强制复制消除不可用时的等价回退"
    evidence: [ev-stdexec-copy-elision-box-init-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++17 之前，复制消除只是编译器被允许做的优化。即使编译器实际省掉了拷贝，语言规则仍要求拷贝或移动构造函数可见且可调用。返回一个删除了拷贝和移动构造的类型，代码在标准层面就是非法的，哪怕编译器并不真的调用它。

C++17 重写了值类别模型：prvalue 不再表示一个临时对象，而是“用于初始化对象的表达式”。用它初始化目标对象时，目标直接在原地构造，中间不存在需要搬移的对象。这条规则是强制的，不依赖优化开关，也不要求类型可移动。标准提案为 P0135R1。

## 基础用法

返回 prvalue 并用它初始化对象，全程不需要拷贝或移动构造。

<!-- compile -->

```cpp
#include <iostream>

struct Immovable {
  Immovable() = default;
  Immovable(const Immovable &) = delete;
  Immovable(Immovable &&) = delete;
  int value = 7;
};

Immovable make() { return Immovable{}; }

int main() {
  Immovable item = make();

  std::cout << item.value << '\n';
}
```

输出 `7`。`Immovable` 删除了拷贝与移动构造，`Immovable item = make();` 仍然合法：`make()` 的返回值是 prvalue，直接初始化 `item` 的存储。同样规则适用于成员初始化、函数实参和 `return Immovable{};`。

## 进阶用法

需要区分两类情况。返回**具名局部变量**属于 NRVO（命名返回值优化），C++17 仍只允许而不强制，且要求类型可移动或可拷贝；返回**临时对象**才是保证的复制消除。

`return std::move(local);` 反而有害。它把具名变量变成 xvalue，阻止 NRVO，并强制要求移动构造。直接写 `return local;` 才是正确写法。

用 `std::move` 包装返回的 prvalue 同样会破坏保证。`T x = std::move(make());` 中的表达式是 xvalue，目标对象只能由移动构造初始化。

保证的复制消除只决定“对象在哪里构造”，不改变类型属性。类型仍然不是 trivially copyable，也不能放进需要移动的容器。它解决的是返回与初始化，不是容器存储。

## 真实项目证据

| 项目    | 位置                                                                                                                                                                                                       | 用法                                                                            | 为什么典型                                         |
| ------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------- | -------------------------------------------------- |
| stdexec | [`include/stdexec/__detail/__basic_sender.hpp#L80-L88`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__basic_sender.hpp#L80-L88)               | 在 requires 表达式中用 prvalue 初始化不可移动类型，检测 decay-copyable 是否成立 | 约束只有在复制消除保证成立时才能满足               |
| stdexec | [`include/stdexec/__detail/__connect_awaitable.hpp#L169-L176`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__connect_awaitable.hpp#L169-L176) | 注释指出成员直接初始化本应触发复制消除，GCC 未实现，退回 `__manual_lifetime`    | 展示规则适用于成员初始化，以及编译器缺陷带来的回退 |
| stdexec | [`include/stdexec/__detail/__any.hpp#L481-L489`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__any.hpp#L481-L489)                             | 同样的初始化在 GCC 上失败时改用 placement new 在 union 成员上直接构造           | placement new 与保证复制消除效果等价，可作为回退   |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。它们都来自同一仓库，且后两条是针对 GCC 未完整实现的规避路径，因此本条目标定为 B 级；特性本身的规则由标准保证，证据只反映生产代码如何依赖它。

## 陷阱与边界

**具名返回值不在保证范围内。** `T f() { T local; return local; }` 依赖 NRVO，类型必须可移动或可拷贝。C++17 不强制编译器消除这一次搬移。

**`std::move` 会阻止优化。** 返回局部变量时加 `std::move` 既关闭 NRVO，又要求移动构造，是常见的负优化。

**保证的是构造位置，不是零成本。** 对象仍要构造、仍要析构；消除的是额外的拷贝或移动，不是构造本身。

**编译器实现可能不完整。** stdexec 的两处证据都说明 GCC 在成员初始化等场景曾拒绝本应保证的消除。跨编译器库需要准备 placement new 或手动生命周期回退。

**不可移动类型仍受容器限制。** `std::vector<Immovable>` 无法扩容，`std::optional` 的赋值也需要可移动或可拷贝。复制消除不解除这些约束。

**值类别判断容易出错。** 条件表达式、逗号表达式、`static_cast` 都可能把 prvalue 变成 xvalue 或 lvalue，从而失去保证。需要依赖消除时，保持初始化式是纯粹的 prvalue。

## 库联动

- **stdexec**：发送者、接收者与状态对象大量不可移动，工厂函数依赖保证复制消除把 prvalue 直接放进成员或约束检查。
- **工厂函数**：`std::make_unique`、`std::make_shared` 返回 prvalue，调用方直接初始化即享受消除；改成先存临时再移动就多一次操作。
- **`std::optional` / `std::variant`**：`emplace` 与原地构造避免临时对象，和复制消除解决的是同一类“少搬一次”的问题。
- **协程与 awaitable**：awaitable 包装器常不可移动，协程帧与包装器的初始化必须依赖原地构造。

## 横向对比

| 方案                   | 是否强制 | 需要可移动/可拷贝 | 适用对象           | 主要代价                 |
| ---------------------- | -------- | ----------------- | ------------------ | ------------------------ |
| 保证的复制消除         | 是       | 否                | prvalue 初始化目标 | 只覆盖 prvalue 场景      |
| NRVO                   | 否       | 是                | 具名局部变量返回   | 依赖编译器，规则不保证   |
| `std::move` 加移动构造 | 不适用   | 是                | 任意               | 多一次搬移，且关闭 NRVO  |
| placement new 手动构造 | 不适用   | 否                | 已知存储地址       | 需手工管理生命周期与对齐 |

跨标准演进：C++17 通过 P0135R1 把 prvalue 初始化改成强制原地构造；C++20 的协程与 concepts 直接建立在这套值类别模型上，不可移动类型因此可以在接口中正常传递。
