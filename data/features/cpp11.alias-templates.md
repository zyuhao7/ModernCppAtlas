---
id: cpp11.alias-templates
title: 别名模板（alias templates）
standard: C++11
kind: language
category: [模板, 类型工具]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.7", clang: "3.0", msvc: "18.0"}
tags: [alias template, using, 类型别名, 模板]
alternatives: [typedef, 继承, 显式特化]
proposal: N2258
last_verified: "2026-11-04"
claims:
  - id: c1
    statement: "用别名模板包装格式化库的类型与 remove_cvref"
    evidence: [ev-spdlog-alias-format-string-001]
  - id: c2
    statement: "用别名模板从 tuple 类型推导下标序列"
    evidence: [ev-fmt-alias-index-sequence-002]
  - id: c3
    statement: "用别名模板固定容器的哈希策略"
    evidence: [ev-drogon-alias-safe-string-map-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

`typedef` 不能带模板参数，想为 `std::vector<std::vector<T>>` 或带自定义哈希的 map 起短名字，只能写一个包装类模板。包装类会引入新的类型身份，构造、推导和重载都要额外适配。

别名模板用 `using Name = ...` 直接给参数化的类型表达式起名，结果与展开后的类型是同一个类型，不产生新的类。标准提案为 N2258，语法属于核心语言，不需要额外头文件。

## 基础用法

在 `using` 左侧写模板参数，右侧写依赖这些参数的类型。使用时像普通模板一样提供实参。

<!-- compile -->

```cpp
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

template <typename T>
using Matrix = std::vector<std::vector<T>>;

template <typename Key, typename Value>
using Table = std::unordered_map<Key, Value>;

template <typename T>
using remove_cvref_t =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;

int main() {
  Matrix<int> grid{{1, 2}, {3, 4}};
  Table<std::string, int> ages{{"atlas", 3}, {"modern", 11}};

  std::cout << grid.size() << ' ' << grid[1][1] << '\n';
  std::cout << ages.size() << ' ' << ages.at("modern") << '\n';

  static_assert(std::is_same<remove_cvref_t<const int&>, int>::value,
                "alias template strips cv and reference");
  std::cout << std::is_same<remove_cvref_t<const int&>, int>::value << '\n';
}
```

输出 `2 4`、`2 11` 与 `1`。完整版本见 [`examples/cpp11/alias_templates.cpp`](../../examples/cpp11/alias_templates.cpp)，六个场景的输出为：

```text
[1] rows = 2, cell = 4, same as vector<vector<int>> = 1
[2] entries = 2, modern = 11
[3] find(alice) = hit, at(ALICE) = 90
[4] twice(21) = 42, enable_if_t<true, int> is int = 1
[5] remove_cvref_t<const int&> is int = 1
[6] Fn<int, int>(2, 3) = 5
```

场景 1 用 `std::is_same` 证明 `Matrix<int>` 与 `std::vector<std::vector<int>>` 是同一个类型，别名没有引入新的类型身份。场景 2 的 `Table<std::string, int>` 只是给键值容器起了短名。场景 3 展示别名的实用能力——把哈希与相等策略固定在右侧，`CiMap` 的键忽略大小写，`find("alice")` 能命中以 `"Alice"` 插入的项。场景 4 说明别名不能特化：先写 `enable_if_impl` 类模板做偏特化，再用 `enable_if_t` 把 `typename ...::type` 收短，`twice` 因此只接受整型。场景 5 的 `remove_cvref_t` 把 `const int&` 还原为 `int`。场景 6 的 `Fn<int, int>` 用参数包转发签名。

`remove_cvref_t` 的名字模仿 C++20 的 `std::remove_cvref_t`；在 C++11 项目中常由库自己补一个，升级标准后可以直接切换到标准库版本。

## 进阶用法

别名模板可以转发参数包。`template <typename... Args> using format_string_t = fmt::format_string<Args...>;` 保留任意数量的实参，调用方不需要知道底层模板的完整参数列表。

别名模板本身不能被特化，也不能做偏特化；需要按类型分支时，仍然要写一个类模板做特化，再用别名模板把结果包起来。这是别名模板与类模板最重要的能力差异。

别名模板不会改变类型身份。`Matrix<int>` 与 `std::vector<std::vector<int>>` 在重载解析、`typeid` 和模板推导中完全等价，因此不能靠别名模板区分两个接口。

在元编程中，别名模板常用于缩短 `typename ...::type` 链。例如把 `typename std::enable_if<B, T>::type` 包成 `enable_if_t<B, T>`，C++14 的标准库正是这样做的；C++11 项目可以自行补一个。

别名模板也可以包装带默认参数的模板，把实现细节（如自定义哈希、分配器）固定在别名里，只把业务相关的参数暴露给调用方。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                               | 用法                                       | 为什么典型                                                  |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------ | ----------------------------------------------------------- |
| spdlog | [`include/spdlog/common.h#L180-L184`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/common.h#L180-L184)                            | 包装 fmt 的格式化类型并补 `remove_cvref_t` | 参数包被完整转发，调用方只写短名字                          |
| fmt    | [`include/fmt/ranges.h#L164-L165`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/ranges.h#L164-L165)                                     | 从 tuple 类型推导下标序列                  | 把嵌套的 `tuple_size` 与 `make_index_sequence` 收成一个名字 |
| drogon | [`lib/inc/drogon/utils/Utilities.h#L792-L794`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/utils/Utilities.h#L792-L794) | 固定键类型与自定义哈希                     | 业务代码不必重复三个模板参数                                |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产头文件，覆盖参数包转发、元编程表达式简化与策略绑定。

## 陷阱与边界

**别名模板不能特化。** 需要偏特化或按条件选择实现时，先写类模板，再用别名模板包装结果；直接对别名模板写特化会编译失败。

**别名不是新类型。** 它不会创建独立的类型身份，重载解析看到的是展开后的类型；想用强类型区分同名底层类型，需要 `enum class` 或包装结构体。

**错误信息会展开。** 编译失败时编译器通常显示别名展开后的完整类型，深层嵌套的别名会让诊断变长；这是可读性与诊断清晰度之间的取舍。

**C++11 缺少标准 `_t` 别名。** `std::decay_t` 等别名是 C++14 才加入的，`std::remove_cvref_t` 到 C++20 才有；C++11 代码要自己写 `typename ...::type` 或者自定义别名。

**别名模板不能推导。** 作为函数参数时，别名模板本身不参与模板实参推导；需要从实参推导时，直接写底层模板或改用类模板。

**命名冲突。** 与 `typedef` 不同，别名模板可以和类模板同名（`using` 引入），同一作用域下要注意查找规则与 ADL。

## 库联动

- **`<type_traits>`**：`enable_if_t`、`decay_t` 等是别名模板最常见的用途；C++11 项目常自行补齐。
- **标准容器**：为嵌套容器、带自定义哈希的 map 起短名字，减少业务代码的模板噪音。
- **fmt / spdlog**：格式化字符串类型通过别名模板转发参数包，调用方无需了解底层实现。
- **drogon**：工具层用别名模板绑定哈希策略，业务层只关心键值类型。
- **Abseil / nlohmann/json**：在 C++11 兼容层中自带一组 `_t` 别名，升级标准后可切回 `std::`。

## 横向对比

| 方案       | 可带模板参数 | 产生新类型 | 可特化 | 主要代价                         |
| ---------- | ------------ | ---------- | ------ | -------------------------------- |
| 别名模板   | 是           | 否         | 否     | 无法按类型分支，诊断会展开       |
| `typedef`  | 否           | 否         | 否     | 不能参数化，只能给具体类型起名   |
| 包装类模板 | 是           | 是         | 是     | 需要转发构造与接口，类型身份不同 |
| 继承       | 是           | 是         | 是     | 引入基类关系，可能影响布局与重载 |

跨标准演进：C++11 通过 N2258 引入别名模板；C++14 用别名模板补齐 `_t` 系列工具；C++17 增加更多 `_v` 变量模板与 `_t` 别名；其“同一类型、不产生新类型”的语义没有变化。
