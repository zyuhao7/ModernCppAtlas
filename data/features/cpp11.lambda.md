---
id: cpp11.lambda
title: lambda 表达式
standard: C++11
kind: language
category: [函数式, 语法]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.5", clang: "3.1", msvc: "16.0"}
tags: [lambda, 闭包, 捕获, 回调, 函数对象]
alternatives: [函数指针, 手写仿函数, std::bind, 具名函数]
proposal: N2927
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用捕获 this 的 lambda 把成员调用包装成延迟执行的刷新回调"
    evidence: [ev-spdlog-lambda-registry-001]
  - id: c2
    statement: "用谓词 lambda 作为条件变量的等待条件"
    evidence: [ev-spdlog-lambda-mpmc-wait-002]
  - id: c3
    statement: "用按值捕获 key 的 lambda 构造生命周期更长的超时回调"
    evidence: [ev-drogon-lambda-cache-timeout-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 之前，要把“一段行为”传给算法或回调接口，只有几种选择：写一个具名函数，但无法携带上下文；手写仿函数类，样板代码多；用 `std::bind` 拼装，语法晦涩且类型推导差；或者用函数指针加 `void*` 上下文，丢掉类型安全。回调逻辑往往只有一两行，却要在远处定义一个完整类型。

lambda 表达式在调用点直接写出匿名函数对象，并显式声明要捕获的外部变量。编译器为每个 lambda 生成唯一的闭包类型，捕获的变量成为该类型的成员。标准提案为 N2927。

## 基础用法

方括号是捕获列表，圆括号是参数列表，花括号是函数体。没有捕获的 lambda 可以转换为函数指针。

<!-- compile -->

```cpp
#include <algorithm>
#include <iostream>
#include <vector>

int main() {
  std::vector<int> values{3, 1, 4, 1, 5};
  std::sort(values.begin(), values.end(), [](int a, int b) { return a > b; });

  int threshold = 2;
  auto above = [threshold](int value) { return value > threshold; };

  long total = 0;
  std::for_each(values.begin(), values.end(),
                [&total](int value) { total += value; });

  const auto count = std::count_if(values.begin(), values.end(), above);
  std::cout << count << ' ' << total << '\n';
}
```

输出 `3 14`：排序按降序进行，`above` 按值保存了 `threshold`，`total` 通过引用被累加。完整版本见 [`examples/cpp11/lambda.cpp`](../../examples/cpp11/lambda.cpp)。

## 进阶用法

捕获列表决定闭包如何持有外部状态。`[=]` 按值捕获用到的变量，`[&]` 按引用捕获，`[x, &y]` 可以逐个指定。按值捕获的变量默认在 lambda 体内是 `const`，要修改副本需加 `mutable`。捕获发生在 lambda 创建时，之后外部变量的变化不会反映到按值捕获的副本上。

C++14 增加初始化捕获，可以写 `[n = 0]` 或把 `std::move` 的结果放进闭包；同时允许 `auto` 参数形成泛型 lambda。C++17 允许 `constexpr` lambda，C++20 允许模板参数列表和无捕获 lambda 的默认构造。需要递归时，通常先把 lambda 存入 `std::function`，或使用 Y 组合子式的显式自引用参数。

捕获引用意味着闭包的生命周期不能超过被引用对象。把 lambda 交给异步任务、线程或事件循环时，必须确认捕获对象的存活时间；跨线程场景通常改为按值捕获，或捕获 `std::shared_ptr` 延长生命周期。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                  | 用法                                      | 为什么典型                               |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------- | ---------------------------------------- |
| spdlog | [`include/spdlog/details/registry.h#L67-L72`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/registry.h#L67-L72)               | 捕获 `this` 生成周期性刷新回调            | 一行逻辑无需具名仿函数，捕获明确依赖对象 |
| spdlog | [`include/spdlog/details/mpmc_blocking_q.h#L30-L38`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/mpmc_blocking_q.h#L30-L38) | 用 lambda 作为 `wait` 的谓词              | 判断逻辑与等待调用就地绑定，作用域清晰   |
| drogon | [`lib/inc/drogon/CacheMap.h#L528-L541`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/CacheMap.h#L528-L541)                  | 按值捕获 `key` 构造超时回调并存入函数对象 | 回调生命周期长于当前调用，捕获方式关键   |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖延迟任务、条件等待与异步回调三种用途。

## 陷阱与边界

**悬空引用。** `[&]` 捕获的变量随作用域结束而失效；把 lambda 存起来稍后调用时，引用捕获几乎必然出错。异步接口要检查回调的调用时机。

**按值捕获不等于复制所有变量。** 只有 lambda 体内实际用到的变量才会被捕获；`[=]` 也不是“复制整个作用域”。

**`mutable` 改变的是副本。** 加 `mutable` 后修改的是闭包自己的成员，外部变量不受影响；要修改外部变量必须按引用捕获。

**每个 lambda 类型唯一。** 两个文本相同的 lambda 仍是不同类型，不能互相赋值；需要统一类型时用 `std::function` 或函数指针。

**捕获 `this` 不延长对象寿命。** 在成员函数里写 `[this]` 只是保存指针；对象销毁后调用回调是未定义行为。C++20 起隐式 `[=]` 捕获 `this` 被弃用，应显式写出。

**避免在性能关键路径滥用 `std::function`。** 它可能分配堆内存并引入间接调用；泛型算法直接接受 lambda 通常可以内联。

## 库联动

- **`<algorithm>`**：`sort`、`find_if`、`count_if`、`for_each` 等接受谓词或变换函数，lambda 是最常用的实参。
- **`std::function`**：需要类型擦除、存储异构回调时把 lambda 装进去，代价是一次可能的分配和间接调用。
- **并发设施**：`std::thread`、`std::async`、条件变量谓词都用 lambda 表达线程入口与等待条件。
- **spdlog / drogon**：日志库用 lambda 注册周期刷新，Web 框架用 lambda 封装缓存超时回调。

## 横向对比

| 方案            | 携带上下文 | 类型安全 | 可内联 | 主要限制                       |
| --------------- | ---------- | -------- | ------ | ------------------------------ |
| lambda          | 是         | 是       | 通常可 | 每个闭包类型唯一               |
| 函数指针        | 否         | 是       | 视情况 | 需要 `void*` 传递上下文        |
| 手写仿函数      | 是         | 是       | 可     | 样板代码多，远离调用点         |
| `std::bind`     | 是         | 较弱     | 较难   | 占位符语法晦涩，易产生悬空引用 |
| `std::function` | 是         | 是       | 否     | 类型擦除带来分配与间接调用     |

跨标准演进：C++11 通过 N2927 引入 lambda；C++14 增加泛型 lambda 与初始化捕获；C++17 允许 `constexpr` lambda；C++20 增加模板参数列表、无捕获 lambda 的默认构造，并弃用隐式捕获 `this`。
