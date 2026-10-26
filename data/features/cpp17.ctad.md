---
id: cpp17.ctad
title: 类模板实参推导（CTAD）
standard: C++17
kind: language
category: [模板, 类型推导]
difficulty: 2
status: final
tier: B
compilers: {gcc: "7", clang: "5", msvc: "19.14"}
tags: [CTAD, 类模板实参推导, 推导指引, initializer_list]
alternatives: [显式模板实参, make_pair 与 make_tuple, 类型别名, C++20 聚合 CTAD]
proposal: P0091R3
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "用推导指引让包装函数指针的类模板从实参推导模板参数"
    evidence: [ev-abseil-ctad-function-ref-001]
  - id: c2
    statement: "为作用域清理类型提供推导指引，同时保留旧工厂函数"
    evidence: [ev-abseil-ctad-cleanup-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++17 之前，类模板的模板实参几乎总要在调用点写出来：`std::pair<int, double> p{1, 2.0}`。工厂函数能省掉一部分，但每种类模板都要配一个 `make_*`，而且工厂函数的返回类型仍然是显式写死的。

CTAD（class template argument deduction）让编译器从构造实参推导类模板参数。构造函数本身会生成隐式推导指引；库作者也可以写显式推导指引，表达构造函数无法覆盖的映射关系。

## 基础用法

<!-- compile -->

```cpp
#include <iostream>
#include <utility>
#include <vector>

template <typename T> struct Box {
  T value;
};

template <typename T> Box(T) -> Box<T>;

int main() {
  std::pair p{1, 2.5};
  std::vector v{1, 2, 3};
  Box box{42};
  std::cout << p.first << ' ' << v.size() << ' ' << box.value << '\n';
}
```

`std::pair p{1, 2.5}` 推导为 `std::pair<int, double>`；`std::vector v{1, 2, 3}` 走 `initializer_list` 指引，推导为 `std::vector<int>`；`Box` 的显式指引把实参类型 `T` 映射到 `Box<T>`。

推导只发生在没有显式模板实参时。写 `std::pair<int, double> p{...}` 仍然完全合法，CTAD 不是强制的新语法。

## 真实项目证据

| 项目       | 位置                                                                                                                                                                      | 用法                                    | 为什么典型                                                                 |
| ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------- | -------------------------------------------------------------------------- |
| abseil-cpp | [`absl/functional/function_ref.h#L226-L227`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/functional/function_ref.h#L226-L227) | 从函数指针推导 `FunctionRef` 的签名参数 | 构造函数拿不到完整函数类型，显式指引补上从 `F*` 到 `FunctionRef<F>` 的映射 |
| abseil-cpp | [`absl/cleanup/cleanup.h#L120-L124`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/cleanup/cleanup.h#L120-L124)                 | 为 `Cleanup` 提供 C++17 推导指引        | 新写法与 C++11 的 `MakeCleanup` 工厂并存，调用方按标准选择                 |

两条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。两者都是公共头文件中的用户定义推导指引，不是测试代码。

## 陷阱与边界

**CTAD 只适用于类模板。** 函数模板的模板实参仍然要从函数参数推导，或者显式写出；不能把 CTAD 的写法直接套到函数模板上。

**`initializer_list` 会改变推导结果。** `std::vector v{1, 2, 3}` 推导为 `std::vector<int>`，而 `std::vector v{"a", "b"}` 推导为 `std::vector<const char*>`，不是 `std::vector<std::string>`。需要字符串容器时显式写类型，或改用 `std::string` 元素。

**聚合类型在 C++17 没有隐式指引。** 没有构造函数的聚合类模板，C++17 需要手写推导指引；C++20 的聚合 CTAD 才让自定义聚合类模板普遍能省略模板实参。abseil 的 `Overload` 就保留了显式指引并注释了这层兼容原因。

**推导指引参与重载决议。** 指引不是宏或注释，它和构造函数一样参与候选集；多个指引都可能匹配时，要按普通重载规则检查可行性与最优匹配，不能假设“后写的赢”。

**指引必须对调用点可见。** 用户定义指引要和类模板放在同一命名空间，并在使用前声明；只在某个实现文件里定义的指引不会影响其他翻译单元。

**显式模板实参会关闭推导。** 一旦写出 `Box<int> b{42}`，编译器不再寻找指引；混用显式和推导写法时，要确认模板参数与实参类型一致。

## 横向对比

| 方案                       | 调用方写法                            | 类型是否显式 | 主要代价                                     |
| -------------------------- | ------------------------------------- | ------------ | -------------------------------------------- |
| 显式模板实参               | `std::pair<int, double> p{...}`       | 是           | 冗长，类型变化时要同步修改                   |
| `make_pair` / `make_tuple` | `auto p = std::make_pair(...)`        | 否           | 需要为每种类模板提供工厂；返回类型可能被调整 |
| CTAD                       | `std::pair p{...}`                    | 否           | 依赖 C++17；`initializer_list` 场景容易推错  |
| 类型别名                   | `using Pair = std::pair<int, double>` | 是           | 别名本身仍需维护，跨模块共享要放在头文件     |

跨标准演进：C++17 通过 P0091R3 引入 CTAD；C++20 通过 P1816R0 增加聚合类模板的推导，让更多无构造函数类型可以直接省略模板实参。
