---
id: cpp11.default-delete
title: = default 与 = delete
standard: C++11
kind: language
category: [面向对象, 语法]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "18.0"}
tags: [= default, = delete, 特殊成员函数, 不可复制, 重载决议]
alternatives: [手写特殊成员函数, 私有未定义声明, 继承 boost::noncopyable, 组合]
proposal: N2346
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用 = delete 显式禁用单例注册表的复制"
    evidence: [ev-spdlog-default-delete-registry-001]
  - id: c2
    statement: "用 = delete 禁止拥有线程的周期任务被复制"
    evidence: [ev-spdlog-default-delete-periodic-worker-002]
  - id: c3
    statement: "用 = default 显式声明可复制且可移动的解析器"
    evidence: [ev-drogon-default-copy-move-multipart-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++ 会隐式生成默认构造、析构、拷贝和移动操作。大多数时候这很方便，但有两种相反的需求：一是希望保留编译器生成的语义，同时在代码里明确写出来，让读者知道这是刻意选择；二是希望彻底禁止某些操作，例如禁止复制持有线程或文件句柄的对象。C++11 之前，“禁止复制”只能把拷贝构造声明为 `private` 且不定义，错误要等到链接期才暴露，也不适用于普通自由函数。

`= default` 要求编译器生成默认实现，`= delete` 把函数声明为“已删除”：调用会在编译期报错，且删除的函数仍参与重载决议。两者都可以用于特殊成员函数和普通函数。标准提案为 N2346。

## 基础用法

在函数声明后写 `= default` 或 `= delete`。删除的函数不写函数体。

<!-- compile -->

```cpp
#include <iostream>

class NonCopyable {
 public:
  NonCopyable() = default;
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
  virtual ~NonCopyable() = default;
};

class Point {
 public:
  Point() = default;
  Point(int x, int y) : x_(x), y_(y) {}
  Point(const Point&) = default;
  Point& operator=(const Point&) = default;

  int x() const { return x_; }
  int y() const { return y_; }

 private:
  int x_ = 0;
  int y_ = 0;
};

int main() {
  Point a(1, 2);
  Point b = a;
  NonCopyable n;
  (void)n;

  std::cout << b.x() << ' ' << b.y() << '\n';
}
```

输出 `1 2`。`Point` 显式默认了拷贝操作，`NonCopyable` 删除拷贝后任何复制尝试都会编译失败。完整版本见 [`examples/cpp11/default_delete.cpp`](../../examples/cpp11/default_delete.cpp)。

## 进阶用法

特殊成员函数的生成规则很微妙。声明任何拷贝操作会抑制隐式移动操作的生成；声明析构函数也会抑制移动。需要“可拷贝且可移动”时，最稳妥的做法是把五个特殊成员都显式写出：析构、拷贝构造、拷贝赋值、移动构造、移动赋值，按需 `= default` 或 `= delete`。

`= delete` 也可用于普通函数和重载。例如删除某个重载以阻止隐式转换：`void log(int) = delete;` 之后传 `int` 会编译失败，而其它重载仍可用。删除的函数参与重载决议，因此它比“私有未定义”更能给出清晰错误，也不会拖到链接期。

`= default` 可以写在类内，也可以写在类外（仅限特殊成员函数）。类内默认通常是隐式 `inline`；类外定义要走正常链接规则。虚析构可以写 `virtual ~Base() = default;`，派生类再用 `override = default` 明确覆写。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                  | 用法                       | 为什么典型                             |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------- | -------------------------------------- |
| spdlog | [`include/spdlog/details/registry.h#L27-L31`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/registry.h#L27-L31)               | 删除注册表的拷贝构造与赋值 | 全局单例不应被复制，约束交给编译器     |
| spdlog | [`include/spdlog/details/periodic_worker.h#L41-L43`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/periodic_worker.h#L41-L43) | 删除持有线程的周期任务拷贝 | 对象拥有 `std::thread`，复制会重复管理 |
| drogon | [`lib/inc/drogon/MultiPart.h#L122-L127`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/MultiPart.h#L122-L127)                | 显式默认拷贝与移动构造     | 默认语义已足够，写出意图并保留移动能力 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖禁止复制与显式默认两种用途。

## 陷阱与边界

**声明拷贝会抑制移动。** 只写 `= default` 的拷贝构造，编译器不会自动生成移动操作；需要移动语义时必须显式默认移动构造与移动赋值。

**`= default` 不等于“一定可复制”。** 如果成员本身不可复制，默认的拷贝操作会被定义为删除；错误仍然出现，只是位置更符合直觉。

**`= delete` 必须写在首次声明处。** 不能先声明再在类外删除；删除的函数不能有函数体，也不能被调用。

**删除的函数仍参与重载决议。** 这既是优点也是陷阱：删除某个重载后，原本会选它的调用直接报错，而不会悄悄退到其它重载。设计重载集时要确认这是期望行为。

**默认移动不清理源对象。** 默认移动构造按成员移动，内置类型成员被复制而不是清零；需要“移后为空”的语义要自己实现。

**不要滥用 `= delete` 表达运行期约束。** 删除只影响编译期可见的调用；通过函数指针或类型擦除仍可能绕过，运行期检查不能省。

## 库联动

- **标准库容器与智能指针**：`std::unique_ptr`、`std::mutex`、`std::thread` 都删除拷贝，只保留移动。
- **RAII 封装**：文件句柄、套接字、锁守卫通常删除拷贝并默认移动，明确所有权唯一。
- **多态基类**：`virtual ~Base() = default;` 配合派生类 `override`，是常见的接口写法。
- **spdlog / drogon**：日志注册表与周期任务用 `= delete` 禁止复制，Web 框架的解析器用 `= default` 保留值语义。

## 横向对比

| 方案                   | 错误时机   | 参与重载决议 | 可表达移动语义 | 主要限制                     |
| ---------------------- | ---------- | ------------ | -------------- | ---------------------------- |
| 隐式生成               | 视使用而定 | 是           | 是             | 意图不明确，规则易被抑制     |
| `= default`            | 编译期     | 是           | 是             | 成员不支持时仍会被删除       |
| `= delete`             | 编译期     | 是           | 不适用         | 只阻止编译期可见的调用       |
| 私有未定义（C++11 前） | 链接期     | 是           | 否             | 错误信息差，不适用于自由函数 |

跨标准演进：C++11 通过 N2346 引入 `= default` 与 `= delete`；C++20 允许在更多场景使用受约束的默认比较，但特殊成员函数的基本规则未变。
