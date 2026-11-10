---
id: cpp11.inheriting-constructors
title: 继承构造函数（inheriting constructors）
standard: C++11
kind: language
category: [初始化, 面向对象]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.8", clang: "3.3", msvc: "19.0"}
tags: [构造函数, 继承, using 声明, 代码复用]
alternatives: [逐一声明转发构造函数, 默认参数, 聚合初始化, 组合替代继承]
proposal: N2540
last_verified: "2026-11-10"
claims:
  - id: c1
    statement: "派生位生成器用 using 引入基类全部构造函数"
    evidence: [ev-abseil-inheriting-ctors-random-001]
  - id: c2
    statement: "引用代理结构体继承别名基类的构造函数"
    evidence: [ev-stdexec-inheriting-ctors-proxy-002]
  - id: c3
    statement: "金额值类型继承基类构造函数，只重写比较运算"
    evidence: [ev-nano-inheriting-ctors-amount-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

派生类通常不继承基类的构造函数：即使基类有 `Base(int)`，`class Derived : public Base` 默认只有默认构造函数和拷贝/移动构造。想让派生对象用同一组参数构造，C++11 之前只能为每个基类构造函数手写一个转发版本。

```cpp
class Derived : public Base {
 public:
  explicit Derived(int value) : Base(value) {}
  explicit Derived(const std::string& label) : Base(label) {}
  // 基类每增加一个构造函数，这里就要同步增加一个
};
```

C++11 引入继承构造函数：在派生类里写 `using Base::Base;`，编译器自动为基类每个可访问构造函数生成对应的派生构造函数。标准提案为 N2540。

## 基础用法

在派生类的 `public` 区写 `using Base::Base;`，即可用基类的构造签名构造派生对象。

<!-- compile -->

```cpp
#include <iostream>
#include <string>

class Base {
 public:
  explicit Base(int value) { std::cout << "int:" << value << '\n'; }

  explicit Base(const std::string& label) {
    std::cout << "str:" << label << '\n';
  }
};

class Derived : public Base {
 public:
  using Base::Base;
};

int main() {
  Derived first(7);
  Derived second(std::string("atlas"));
}
```

输出 `int:7` 与 `str:atlas`。派生类没有手写任何构造函数，两个基类重载都被继承。完整版本见 [`examples/cpp11/inheriting_constructors.cpp`](../../examples/cpp11/inheriting_constructors.cpp)。

## 进阶用法

**继承的是重载集合，不是某个签名。** `using Base::Base;` 把基类全部可访问构造函数一起引入；基类后续新增构造函数时派生类自动获得，无需改动。

**默认构造与拷贝/移动构造仍按规则生成。** 继承构造函数不替代编译器生成的默认、拷贝、移动构造函数；只有显式声明的构造会抑制它们。

**模板基类也能继承。** `using AliasBase<T>::AliasBase;` 可以继承模板别名基类的构造，stdexec 的引用代理就用这种方式。

**与成员初始化结合。** 派生类新增成员时，继承构造函数会先调用基类构造，再默认初始化新增成员；需要自定义初始化时仍要手写构造函数。

**`using` 也引入其他成员。** 同一个 `using Base::operator=;`、`using Base::min;` 可以一并引入赋值运算符与静态成员，abseil 的位生成器就是构造与成员一起继承。

**继承构造函数不参与聚合初始化。** 一旦使用 `using Base::Base;`，派生类就有用户声明的构造函数；需要聚合初始化的场景不能用。

## 真实项目证据

| 项目    | 位置                                                                                                                                                                           | 用法                                       | 为什么典型                                     |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------ | ---------------------------------------------- |
| abseil  | [`absl/random/random.h#L103-L112`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/random/random.h#L103-L112)                          | 位生成器用 `using Base::Base` 引入全部构造 | 派生类只加别名与静态成员，构造完全复用         |
| stdexec | [`include/stdexec/__detail/__any.hpp#L643-L649`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__any.hpp#L643-L649) | 引用代理结构体继承模板别名基类的构造       | 模板别名基类的签名无法逐一转发                 |
| nano    | [`nano/lib/numbers.hpp#L93-L101`](https://github.com/nanocurrency/nano-node/blob/6b5f63ce04c9a1ffe37b8c687fffc1bef941bf98/nano/lib/numbers.hpp#L93-L101)                       | 金额值类型继承基类构造，只重写比较         | 派生类型保留全部构造入口，仅覆盖需要变化的行为 |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖普通基类、模板别名基类与值类型三种继承形态。

## 陷阱与边界

**私有基类构造函数不会被继承。** 只继承派生类有权访问的基类构造函数；基类私有构造函数不可用。

**继承构造函数是“隐式声明”的。** 它们不参与用户声明的构造函数计数，但会参与重载决议；出现签名冲突时按普通重载规则处理。

**与 `explicit` 的传播。** 基类构造函数是 `explicit` 时，继承来的版本也保持 `explicit`；不会因为继承而变成隐式转换。

**默认参数在继承后不可见。** 继承构造函数按基类构造函数的实际参数列表生成，默认参数不会成为派生构造函数的默认参数；调用时仍需显式给出。

**不能继承初始化列表构造函数。** 基类的 `std::initializer_list` 构造函数不会被 `using` 带入，需要时仍要手写。

**MSVC 支持较晚。** 继承构造函数要到 VS2015（19.0）才可用；更老的工具链只能逐条转发。

## 库联动

- **`using` 声明**：继承构造函数与 `using Base::operator=`、`using Base::min` 同属成员引入机制，可组合使用。
- **模板别名基类**：`using Alias<T>::Alias;` 是处理模板基类构造的标准写法。
- **`std::initializer_list`**：列表初始化构造函数不参与继承，需要单独声明。
- **`explicit`**：构造函数的显式性随继承保留，影响隐式转换与重载决议。
- **值类型包装**：强类型别名常继承基类构造，仅覆盖比较或转换。
- **abseil / stdexec / nano-node**：分别展示普通基类、模板别名基类与值类型三种用法。

## 横向对比

| 方案             | 构造覆盖     | 维护成本             | 主要限制                          |
| ---------------- | ------------ | -------------------- | --------------------------------- |
| 继承构造函数     | 基类全部构造 | 低，基类新增自动生效 | 不继承初始化列表构造，MSVC 支持晚 |
| 逐条转发构造函数 | 手动选择     | 高，需同步维护       | 基类每新增构造都要补一条          |
| 默认参数         | 单构造函数   | 低                   | 不同签名需要不同逻辑时表达力不足  |
| 组合替代继承     | 按需转发     | 中                   | 需要显式暴露成员，接口不再透明    |

跨标准演进：C++11 通过 N2540 引入继承构造函数，解决基类构造转发样板；C++17 的聚合初始化与类模板实参推导继续围绕构造规则演进，但不改变继承语义；C++20 的指定初始化器也不能与继承构造函数同时用于聚合初始化。
