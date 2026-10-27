---
id: cpp11.move-semantics
title: 移动语义与右值引用
standard: C++11
kind: language
category: [对象模型, 性能]
difficulty: 4
status: final
tier: A
compilers: {gcc: "4.5", clang: "2.9", msvc: "17.0"}
tags: [move, 右值引用, std::move, 移动构造, 所有权]
alternatives: [拷贝构造, swap 交换, 指针或引用传递, std::unique_ptr]
proposal: N2118
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "构造函数按值接收容器，再用 std::move 移入成员"
    evidence: [ev-mold-move-sink-ctor-001]
  - id: c2
    statement: "在成员初始化列表里移动 unique_ptr，转移所有权"
    evidence: [ev-mold-move-unique-member-002]
  - id: c3
    statement: "局部容器不再使用时，用 std::move 把结果交给长期对象"
    evidence: [ev-mold-move-vector-arg-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 之前，对象传递只有拷贝一条路。从函数返回容器、把大对象存进成员、在容器里插入元素，都要复制整份数据；即使源对象马上就要销毁，语言也没有办法表达“资源直接转交”。`std::auto_ptr` 尝试过所有权转移，但拷贝语义混乱，最终被废弃。

移动语义把表达式分成左值和右值：左值有名字、可以反复使用；右值是将要销毁的临时对象或显式标记为可转移的对象。类型可以提供移动构造函数与移动赋值运算符，直接接管源对象的内部资源，把源对象留在“有效但未指定”的状态。标准提案为 N2118。

## 基础用法

`std::move` 不移动任何东西，它只是把表达式转换成右值引用，让重载决议选中移动构造。

<!-- compile -->

```cpp
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Buffer {
public:
  explicit Buffer(std::vector<int> values) : values_(std::move(values)) {}

  std::size_t size() const { return values_.size(); }

private:
  std::vector<int> values_;
};

int main() {
  Buffer buffer({1, 2, 3});
  std::string text = "atlas";
  std::string moved = std::move(text);

  std::cout << buffer.size() << ' ' << moved << '\n';
}
```

输出 `3 atlas`。`Buffer` 按值接收 `vector`，实参构造时先拷贝或移动一次，随后 `std::move(values)` 把这份临时存储直接交给成员，不再复制元素。完整版本见 [`examples/cpp11/move_semantics.cpp`](../../examples/cpp11/move_semantics.cpp)。

## 进阶用法

“按值接收再移动”是 sink 参数的标准写法。调用方传左值时有且只有一次拷贝，传右值时没有拷贝；比同时写 `const T&` 与 `T&&` 两个重载更简单，也更容易保持正确。

移动构造与移动赋值通常标 `noexcept`。`std::vector` 扩容时只有在元素移动构造不抛异常的情况下才会搬运元素，否则退回复制以保证强异常安全。自定义类型若把移动构造写成可能抛异常，容器性能会明显下降。

移动后的对象仍然有效，但值未指定。可以析构、可以重新赋值，不能假设内容为空。需要继续使用时先 `clear()` 或重新赋值，不要依赖某个具体残留值。

`std::move` 作用在 `const` 对象上得到 `const T&&`，无法绑定到 `T&&` 移动构造，最终仍走拷贝。不要把 `const` 对象的 `std::move` 当成优化。

转发引用（`T&&` 且 `T` 由推导得到）需要配合 `std::forward`，按原始值类别转发；`std::move` 会无条件变成右值，用错会意外掏空调用方的对象。

## 真实项目证据

| 项目 | 位置                                                                                                                                              | 用法                                                 | 为什么典型                         |
| ---- | ------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------- | ---------------------------------- |
| mold | [`src/mold.h#L1123-L1124`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/mold.h#L1123-L1124)                   | 构造函数按值接收 `vector`，再用 `std::move` 移入成员 | sink 参数的标准写法，避免多余拷贝  |
| mold | [`src/input-sections.cc#L343-L345`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/input-sections.cc#L343-L345) | 在初始化列表里移动 `unique_ptr`，把所有权交给成员    | move-only 类型只能用移动转移所有权 |
| mold | [`src/cmdline.cc#L461-L464`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/cmdline.cc#L461-L464)               | 局部 `vector` 构建完成后移动进命令行参数             | 局部对象即将销毁，移动代替整份复制 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在 mold 的生产路径上，分别对应 sink 参数、move-only 所有权转移和局部结果转交三种常见场景。

## 陷阱与边界

**移动不是免费的。** 移动构造仍需构造新对象、转移指针或句柄；小对象（几个整数）的移动与拷贝开销接近，甚至因为分支判断更慢。

**不要使用已移动的对象。** 标准只保证“有效但未指定”。读取已移动的 `std::string` 长度、继续 `push_back` 已移动的 `vector`，结果取决于实现。

**`return std::move(local)` 会阻止 NRVO。** 具名局部变量直接 `return local;` 才可能享受返回值优化，加 `std::move` 反而要求移动构造。

**`const` 对象无法被移动。** `std::move(constObj)` 得到 `const T&&`，只能匹配拷贝构造；看似优化，实际没有效果。

**移动构造抛异常会拖慢容器。** `std::vector` 在扩容时倾向复制而非移动可能抛异常的元素。移动构造与移动赋值应尽量标 `noexcept`。

**移动后自赋值需要防御。** 移动赋值运算符要处理 `this == &other` 的情况，否则可能先释放自己的资源再读取同一份资源。

**基类与虚析构仍需遵守规则。** 定义了析构函数、拷贝或移动之一时，其余特殊成员函数的隐式生成规则会变化，必要时要显式 `= default`。

## 库联动

- **标准容器与字符串**：`std::vector`、`std::string`、`std::map` 都提供移动构造与移动赋值，扩容和返回时自动受益。
- **智能指针**：`std::unique_ptr` 只能移动，天然表达唯一所有权；`std::shared_ptr` 的移动只转移控制块引用，不增加计数。
- **并发设施**：`std::thread`、`std::promise`、`std::future` 都是 move-only，任务和结果通过移动进入队列。
- **工厂与 sink 接口**：按值接收参数再移动，是库 API 避免重载爆炸的常见选择。

## 横向对比

| 方案              | 是否复制资源 | 源对象状态   | 适用场景               | 主要代价                  |
| ----------------- | ------------ | ------------ | ---------------------- | ------------------------- |
| 拷贝构造          | 是           | 保持不变     | 源对象仍需使用         | 深拷贝开销                |
| 移动构造          | 否           | 有效但未指定 | 源对象即将销毁         | 需要正确实现与 `noexcept` |
| 指针或引用传递    | 否           | 不变         | 只读访问或共享生命周期 | 生命周期管理复杂          |
| `std::unique_ptr` | 否           | 变为空指针   | 明确唯一所有权         | 有间接寻址成本            |

跨标准演进：C++11 通过 N2118 引入右值引用与移动语义；C++17 的保证复制消除进一步减少返回值场景的移动，C++20 的 concepts 让转发引用与约束的组合更易读。
