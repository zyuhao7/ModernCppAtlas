---
id: cpp11.reference-wrapper
title: 引用包装器（std::reference_wrapper）
standard: C++11
kind: library
category: [函数式, 类型工具]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "16.0"}
tags: [reference_wrapper, std::ref, 引用语义, 容器, std::bind]
alternatives: [裸指针, const 引用参数, lambda 捕获引用, std::shared_ptr]
proposal: N1453
last_verified: "2026-11-10"
claims:
  - id: c1
    statement: "用 reference_wrapper 作为队列映射的键"
    evidence: [ev-duckdb-reference-wrapper-queue-key-001]
  - id: c2
    statement: "传函数指针时用 std::ref 包装合并器"
    evidence: [ev-abseil-reference-wrapper-combiner-002]
  - id: c3
    statement: "反射调用返回左值引用时用 std::ref 包装"
    evidence: [ev-glaze-reference-wrapper-api-return-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

标准容器只能存对象，不能存引用：`std::vector<int&>` 无法编译。模板参数按值传递时也会退化，`std::bind`、`std::thread` 与算法会复制实参，调用方对原对象的修改传不回去。裸指针能绕开限制，却丢失类型信息与值语义。

C++11 提供 `std::reference_wrapper<T>`：一个可拷贝、可赋值的小对象，内部保存 `T*`，对外可隐式转换为 `T&`，`get()` 返回底层引用。`std::ref` 与 `std::cref` 是创建它的辅助函数。标准提案为 N1453。

## 基础用法

把引用装进容器后，遍历修改的就是原对象。

<!-- compile -->

```cpp
#include <functional>
#include <iostream>
#include <vector>

int main() {
  int first = 1;
  int second = 2;

  std::vector<std::reference_wrapper<int>> items{first, second};
  for (int& item : items) {
    item *= 10;
  }

  std::cout << first << ' ' << second << '\n';
  std::cout << items[0].get() << ' ' << items[1].get() << '\n';
}
```

输出 `10 20` 与 `10 20`。容器里存的是两个 `int` 的包装，`for (int& item : items)` 通过隐式转换取得原引用，`*= 10` 直接修改 `first` 与 `second`。完整版本见 [`examples/cpp11/reference_wrapper.cpp`](../../examples/cpp11/reference_wrapper.cpp)，五个场景的输出为：

```text
[1] first = 10, second = 20
[2] after by-value thread = 10, after ref thread = 11
[3] total after bind = 12
[4] accumulator total = 3
[5] refs compare by value = true, cref value = 42
```

场景 2 先后启动两个线程：按值接收参数的 `bump_by_value` 只改线程内的副本，原变量仍是 10；`bump(std::ref(value))` 才写回原变量，`join` 后变成 11。这里不能写成 `std::thread(bump, value)`，因为 `bump` 要求 `int&`，而线程先按值保存实参。场景 3 用 `std::bind` 保存 `std::ref(total)`，两次调用都累加回原变量；场景 4 直接调用 `std::ref(accumulator)`，验证包装可调用对象；场景 5 说明默认比较看值，`std::cref` 则提供只读视图。

## 进阶用法

**隐式转换与 `get()` 并存。** `reference_wrapper<T>` 可以自动转成 `T&`，也可以显式调用 `.get()`；传给模板时显式调用更容易避免推导歧义。

**可调用对象也能包装。** 当 `T` 是函数对象时，`reference_wrapper` 提供 `operator()`，直接转发到被包装对象，`std::bind` 与 `std::mem_fn` 依赖这一能力。

**`std::ref` 与 `std::cref` 成对使用。** `std::ref(x)` 得到 `reference_wrapper<T>`，`std::cref(x)` 得到 `reference_wrapper<const T>`；后者阻止通过包装修改原对象。

**按值转发的接口需要它。** `std::thread`、`std::bind`、`std::make_pair` 等会衰减并复制实参，想传递引用必须显式包一层 `std::ref`。

**容器可存引用。** `std::vector<std::reference_wrapper<T>>` 常用于保存对已有对象的观察集合，不拥有也不复制元素。

**默认比较看被引用对象的值。** `reference_wrapper` 的 `==` 与 `<` 委托给被引用对象的值比较，而不是包装对象或地址；两个指向不同对象但值相等的包装会判定相等。若要以地址区分实体，需要像 duckdb 的 `reference_map_t` 那样自定义哈希与相等比较。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                    | 用法                                    | 为什么典型                                       |
| ------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------- | ------------------------------------------------ |
| duckdb | [`src/parallel/task_scheduler.cpp#L146-L150`](https://github.com/duckdb/duckdb/blob/069cc9f9b5be802405797faecc284961b07c70ef/src/parallel/task_scheduler.cpp#L146-L150) | 用 `reference_wrapper` 作为队列映射的键 | 自定义哈希与相等比较按地址建键，引用可直接作为键 |
| abseil | [`absl/hash/hash.h#L432-L436`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/hash/hash.h#L432-L436)                           | 传函数指针时用 `std::ref` 包装合并器    | 回调签名要求传值，包装后不复制已有对象           |
| glaze  | [`include/glaze/api/api.hpp#L121-L128`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/api/api.hpp#L121-L128)        | 反射调用返回左值引用时用 `std::ref`     | 返回类型需要值语义，语义上却要指向调用方对象     |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖容器键、回调参数与返回值三种引用传递场景。

## 陷阱与边界

**不延长生命周期。** `reference_wrapper` 只保存指针；被引用对象销毁后包装立即悬空，不能把临时对象的 `std::ref` 存起来。

**异步接口尤其危险。** 把 `std::ref` 传给线程或延迟回调时，必须保证原对象活得比任务久；abseil 的 `bind_front` 文档专门把这种用法列为错误示例。

**没有默认构造函数。** `reference_wrapper` 必须绑定到已有对象，不能先声明后赋值；容器扩容不会默认构造元素，但 `resize` 等操作无法用于它。

**不要与指针混用语义。** 包装对象按值拷贝，底层仍指向同一实体；想复制数据应直接存值，想共享所有权应使用 `std::shared_ptr`。

**`cref` 只限制通过包装修改。** 原对象仍可被其他非 const 路径修改，`reference_wrapper<const T>` 不是线程安全的只读快照。

**早期 MSVC 行为有差异。** VS2010（16.0）已提供 `std::ref`，但 `reference_wrapper` 在常量表达式与返回值推导上的细节要到后续版本才稳定。

## 库联动

- **`<functional>`**：`reference_wrapper`、`std::ref`、`std::cref`、`std::bind`、`std::mem_fn` 同处一个头文件。
- **`std::thread`**：构造函数按值保存参数，需要引用时用 `std::ref` 显式包装。
- **`std::bind`**：占位符与引用包装配合，把调用方对象按引用绑定到可调用对象。
- **标准容器**：`reference_wrapper` 是容器存放引用的标准手段，元素保持可拷贝、可赋值。
- **`std::invoke`（C++17）**：统一处理函数指针、成员指针与 `reference_wrapper` 的调用语义。
- **duckdb / abseil / glaze**：分别展示映射键、回调参数与反射返回值三种包装位置。

## 横向对比

| 方案                | 可存容器 | 生命周期管理 | 主要限制                         |
| ------------------- | -------- | ------------ | -------------------------------- |
| `reference_wrapper` | 是       | 不拥有       | 必须自行保证被引用对象存活       |
| 裸指针              | 是       | 不拥有       | 丢失类型与值语义，容易空指针     |
| `const T&` 参数     | 否       | 不拥有       | 只能用于函数参数，无法存储或返回 |
| `std::shared_ptr`   | 是       | 共享拥有     | 引入引用计数与分配开销           |

跨标准演进：C++11 通过 N1453 把 TR1 的 `reference_wrapper`、`std::ref` 与 `std::cref` 纳入标准；C++14 与 C++17 只做缺陷修正与 `std::invoke` 的调用统一，包装本身的语义保持稳定。
