---
id: cpp11.initializer-list
title: std::initializer_list
standard: C++11
kind: library
category: [初始化]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.1", msvc: "18.0"}
tags: [initializer_list, 列表初始化, 构造函数, 可变参数]
alternatives: [可变参数模板, std::vector 构造, C 风格数组]
proposal: N2672
last_verified: "2026-11-04"
claims:
  - id: c1
    statement: "用 initializer_list 作为构造参数，让单 sink 重载委托到列表重载"
    evidence: [ev-spdlog-init-list-sinks-001]
  - id: c2
    statement: "在 initializer_list 中展开可变参数包，保证参数按顺序绑定"
    evidence: [ev-drogon-init-list-pack-expansion-002]
  - id: c3
    statement: "构造函数直接接收 initializer_list 作为权重序列"
    evidence: [ev-abseil-init-list-weights-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

函数或构造函数想接受数量不固定的同类型实参，C++03 只能写多个重载、要求调用方先建一个 `std::vector`，或者退回 C 风格数组加长度参数。前两种写法在调用点啰嗦，第三种丢失类型信息且容易越界。

`std::initializer_list<T>` 让花括号列表直接绑定到参数：`total({10, 20, 30})`。它是一段只读元素的轻量视图，提供 `begin`、`end` 和 `size`，可以参与范围 for，也可以被容器和构造函数复制。标准提案为 N2672，头文件是 `<initializer_list>`。

## 基础用法

按值传递 `std::initializer_list<int>` 即可接收任意长度的列表。元素在列表生命周期内只读，需要保存时把它复制进自己的容器。

<!-- compile -->

```cpp
#include <initializer_list>
#include <iostream>
#include <numeric>
#include <vector>

class Numbers {
 public:
  Numbers(std::initializer_list<int> values) : values_(values) {}

  std::size_t size() const { return values_.size(); }

  int sum() const {
    int total = 0;
    for (int value : values_) {
      total += value;
    }
    return total;
  }

 private:
  std::vector<int> values_;
};

int total(std::initializer_list<int> values) {
  return std::accumulate(values.begin(), values.end(), 0);
}

int main() {
  Numbers numbers{1, 2, 3, 4};
  std::cout << numbers.size() << ' ' << numbers.sum() << '\n';
  std::cout << total({10, 20, 30}) << '\n';

  std::initializer_list<int> values{7, 8, 9};
  int sum = 0;
  for (int value : values) {
    sum += value;
  }
  std::cout << values.size() << ' ' << sum << '\n';
}
```

输出 `4 10`、`60` 与 `3 24`。`Numbers` 的构造函数先把列表复制进 `values_`，`total` 直接遍历视图，第三个例子展示 `initializer_list` 也能像普通范围一样使用。完整版本见 [`examples/cpp11/initializer_list.cpp`](../../examples/cpp11/initializer_list.cpp)。

`initializer_list` 本身不拥有元素，底层数组由编译器在调用点创建，生命周期到完整表达式结束。因此不要保存它的 `begin()` 指针，也不要把列表作为长期存储。

## 进阶用法

列表初始化优先选择 `initializer_list` 构造函数：只要类同时有 `std::vector<int>` 构造函数和 `initializer_list<int>` 构造函数，`{1, 2, 3}` 会选后者。这是很多容器接口的设计基础，也是重载解析中最容易踩坑的地方——想调用普通构造函数时必须改用圆括号。

`initializer_list` 的元素类型是 `const T`，不能原地修改；需要修改时先复制到 `std::vector` 或其他容器。列表长度由调用点决定，不能追加元素，也没有 `push_back`。

空列表 `{}` 会匹配 `initializer_list` 构造函数而不是默认构造函数，如果两者都存在且语义不同，要在接口文档里写清楚。C++11 的列表初始化还禁止窄化转换，`std::vector<int>{1.5}` 会直接编译失败，这比运行期截断更安全。

`initializer_list` 常用于小规模常量参数，例如日志 sink 列表、权重表和配置项；大规模数据仍应使用容器或迭代器区间，避免在调用点构造临时数组。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                 | 用法                                             | 为什么典型                                          |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------ | --------------------------------------------------- |
| spdlog | [`include/spdlog/logger.h#L63-L69`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/logger.h#L63-L69)                                  | 单 sink 构造委托到 `initializer_list` 构造       | 两个入口共用一段初始化逻辑，调用方可以写花括号列表  |
| drogon | [`orm_lib/inc/drogon/orm/Mapper.h#L1489-L1492`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/inc/drogon/orm/Mapper.h#L1489-L1492) | 在 `initializer_list` 中展开参数包，固定求值顺序 | 用列表初始化保证每个参数按从左到右的顺序写入 binder |
| abseil | [`absl/random/discrete_distribution.h#L121-L122`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/random/discrete_distribution.h#L121-L122)  | 构造函数直接接收权重列表                         | 常量权重表无需先建容器，与迭代器区间构造并存        |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖构造函数委托、求值顺序控制与参数建模三种用法。

## 陷阱与边界

**不要保存列表的底层数组。** `initializer_list` 指向调用点创建的临时数组，函数返回后继续使用其迭代器或指针是悬垂引用。

**元素只读。** 列表元素类型是 `const T`，需要修改时先复制到 `std::vector`；直接写 `list[0] = x` 会编译失败。

**列表构造函数会抢重载。** `std::vector<int> v{3}` 创建一个元素的 vector，不是三个默认元素；要后者必须写 `std::vector<int>(3)` 或 `std::vector<int>(3, 0)`。

**空花括号有歧义。** `T{}` 在存在 `initializer_list` 构造函数时可能选择列表构造，而不是默认构造；接口设计时要明确空列表的含义。

**列表初始化禁止窄化。** `int x{1.5}` 和 `std::vector<char>{1000}` 都是编译错误；这是有意设计，但迁移旧代码时可能批量报错。

**长度不可变。** `initializer_list` 只是视图，没有 `push_back`；需要动态增长时用容器。

**不是万能的可变参数。** 元素类型必须相同；混合类型应使用可变参数模板或 `std::tuple`。

## 库联动

- **标准容器**：`std::vector`、`std::map`、`std::set` 都提供 `initializer_list` 构造函数，`{...}` 是构造容器的常用写法。
- **`std::accumulate` / 范围算法**：列表天然提供 `begin`/`end`，可以直接传给算法，无需先复制。
- **可变参数模板**：需要异构参数时用参数包；`initializer_list` 适合同类型、数量不定的场景。
- **spdlog**：logger 构造函数用 sink 列表统一单 sink 与多 sink 两种入口。
- **Abseil**：随机分布的权重表用列表直接初始化，减少调用点样板代码。

## 横向对比

| 方案               | 元素类型 | 长度是否可变 | 是否拥有元素 | 主要代价                         |
| ------------------ | -------- | ------------ | ------------ | -------------------------------- |
| `initializer_list` | 单一类型 | 否           | 否           | 底层数组生命周期短，不能长期保存 |
| 可变参数模板       | 可异构   | 编译期固定   | 由实现决定   | 模板实例化成本，错误信息较长     |
| `std::vector`      | 单一类型 | 是           | 是           | 需要堆分配与复制                 |
| C 风格数组加长度   | 单一类型 | 否           | 由调用方决定 | 丢失长度信息，易越界             |

跨标准演进：C++11 通过 N2672 引入列表初始化与 `std::initializer_list`；C++14 放宽了部分 `constexpr` 限制，使列表可用于更多编译期场景；后续标准没有改变其只读视图的本质。
