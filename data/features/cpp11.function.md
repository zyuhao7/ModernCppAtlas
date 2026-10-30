---
id: cpp11.function
title: std::function
standard: C++11
kind: library
category: [函数式, 类型擦除]
difficulty: 2
status: final
tier: B
compilers: {gcc: "4.4", clang: "3.1", msvc: "16.0"}
tags: [std::function, 类型擦除, 回调, 可调用对象, 分配]
alternatives: [函数指针, 虚函数接口, 模板参数, lambda 直接调用]
proposal: N1402
last_verified: "2026-10-30"
claims:
  - id: c1
    statement: "用 std::function 接收异步响应回调并向下游转移"
    evidence: [ev-drogon-function-callback-001]
  - id: c2
    statement: "用 std::function 保存并调用每条日志的回调"
    evidence: [ev-spdlog-function-callback-sink-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

回调需要在运行时才知道具体是可捕获状态的 lambda、函数指针还是带状态的函数对象。裸函数指针无法携带捕获；模板参数会把可调用类型写进周围所有类型，造成代码膨胀并逼着接口暴露实现细节；虚函数接口要给每个回调手写一个子类。

`std::function` 是一个类型擦除的“可调用对象容器”：只要签名兼容，函数指针、lambda、`std::bind` 结果或自定义函数对象都能装进去，以统一类型按值保存和复制。代价是一次可能的堆分配和一次间接调用。标准提案为 N1402。

## 基础用法

模板参数写签名 `R(Args...)`；用 `operator=` 或构造函数放入任意兼容可调用对象，调用语法与普通函数相同。空 `std::function` 调用会抛 `std::bad_function_call`，用 `operator bool()` 判空。

<!-- compile -->

```cpp
#include <functional>
#include <iostream>

int add(int a, int b) { return a + b; }

int main() {
  std::function<int(int, int)> op = add;
  int offset = 10;
  op = [offset](int a, int b) { return a + b + offset; };
  std::cout << op(1, 2) << '\n';
}
```

输出 `13`。第一次装入普通函数，随后换成捕获了 `offset` 的 lambda；调用点代码不变，具体类型被擦除。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                          | 用法                                                    | 为什么典型                                                               |
| ------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------- | ------------------------------------------------------------------------ |
| drogon | [`lib/inc/drogon/HttpBinder.h#L206-L219`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/HttpBinder.h#L206-L219)      | 异步响应回调按右值引用接收后 `std::move` 交给下游       | 用户回调签名固定但状态任意，擦除后才能统一排队和跨层传递                 |
| spdlog | [`include/spdlog/sinks/callback_sink.h#L15-L29`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/sinks/callback_sink.h#L15-L29) | `typedef` 固化回调签名，sink 按值保存并在每条日志上调用 | 日志库不知道用户回调类型，`std::function` 让 sink 统一存储任意可调用对象 |

## 陷阱与边界

**空 `std::function` 调用会抛异常。** 默认构造的 `std::function` 不含目标，直接调用抛 `std::bad_function_call`；调用前用 `if (fn)` 检查，或在接口处禁止传入空值。

**可能有堆分配。** lambda 捕获的对象装得下 `std::function` 的内部小缓冲时不分配；捕获过多或过大就会走堆。热路径上需要关注分配成本，必要时改用模板参数或虚接口。

**会复制被捕获的对象。** 把带状态的 lambda 放入 `std::function` 会复制它的捕获；需要移动语义时用 `std::move` 构造 `std::function`，并要求被调用对象支持移动。

**`std::bind` 不是首选。** C++11 的 `std::bind` 难以阅读且丢失参数类型信息；优先用 lambda 完成同样的绑定，行为和错误信息都更清楚。

**类型擦除丢掉的是类型不是生命周期。** `std::function` 按值保存目标，目标引用的外部对象仍需自己保证存活；常见错误是放入了捕获 `this` 的 lambda，对象先于回调析构。

**重载函数要显式转换。** `std::function<int(int)> f = overload_set;` 无法推导；先转成具体函数指针再放入。

## 横向对比

| 方案            | 可携带状态 | 运行时开销          | 接口侵入性             | 主要限制                       |
| --------------- | ---------- | ------------------- | ---------------------- | ------------------------------ |
| `std::function` | 是         | 可能分配 + 间接调用 | 低，签名固定           | 类型擦除成本，空调用抛异常     |
| 函数指针        | 否         | 一次间接调用        | 低                     | 无法捕获状态                   |
| 虚函数接口      | 是         | 虚表间接调用        | 高，需为每个回调写子类 | 类型层次僵硬，复制语义要自己管 |
| 模板参数        | 是         | 可内联，通常最优    | 高，类型扩散到调用方   | 代码膨胀，无法放在统一容器里   |

跨标准演进：C++11 通过 N1402 引入 `std::function`，同时 `std::bind` 与 lambda 一并到位；C++17 起 `std::function` 可以从函数指针构造并支持 `constexpr` 的一部分用法；C++23 增加 `std::move_only_function`，用于承载不可复制的可调用对象。
