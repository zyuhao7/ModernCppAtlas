---
id: cpp11.forwarding-references
title: 转发引用与完美转发（forwarding references）
standard: C++11
kind: language
category: [模板, 类型系统]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.3", clang: "2.9", msvc: "16.0"}
tags: [转发引用, 完美转发, std::forward, 引用折叠, 值类别]
alternatives: [为左值与右值各写一个重载, 按值传递后 std::move, const 引用拷贝, 直接暴露底层引用]
proposal: N2118
last_verified: "2026-11-10"
claims:
  - id: c1
    statement: "make_unique 用 Args&& 与 std::forward 完美转发构造参数"
    evidence: [ev-spdlog-forwarding-reference-make-unique-001]
  - id: c2
    statement: "构造函数用 FUNCTION&& 与 std::forward 保存可调用对象"
    evidence: [ev-drogon-forwarding-reference-http-binder-002]
  - id: c3
    statement: "输出代理模板用 T&& 与 std::forward 转交参数"
    evidence: [ev-mold-forwarding-reference-sync-stream-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

模板函数想把参数原样传给下一层时，用 `const T&` 会丢掉右值性，所有实参退化为左值，只能拷贝；用 `T&` 又收不了临时对象。C++11 之前只能为左值和右值各写一个重载，参数个数一多就无法穷举。

```cpp
void relay(const std::string& value) { sink(value); }        // 右值也只能拷贝
void relay(std::string&& value) { sink(std::move(value)); }  // 每种值类别各写一份
```

C++11 引入转发引用：模板参数写成 `T&&` 且 `T` 由该函数调用推导时，左值实参推导为 `T&`，右值实参推导为 `T`，再交给 `std::forward<T>` 按原始值类别转发。标准提案为 N2118。

## 基础用法

`T&&` 只有在模板推导语境下才是转发引用；配合 `std::forward<T>` 把值类别保留到下一层。

<!-- compile -->

```cpp
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Buffer {
 public:
  template <typename T>
  void add(T&& value) {
    store_.push_back(std::forward<T>(value));
  }

  const std::vector<std::string>& items() const { return store_; }

 private:
  std::vector<std::string> store_;
};

int main() {
  Buffer buffer;
  std::string text = "atlas";
  buffer.add(text);
  buffer.add(std::string("moved"));

  for (const auto& item : buffer.items()) {
    std::cout << item << '\n';
  }
  std::cout << text << '\n';
}
```

输出 `atlas`、`moved` 与 `atlas`。第一次调用传入左值，`T` 推导为 `std::string&`，参数按引用绑定并拷贝进容器；第二次传入临时对象，`T` 推导为 `std::string`，参数按右值引用绑定并移动进容器，`text` 本身保持不变。完整版本见 [`examples/cpp11/forwarding_references.cpp`](../../examples/cpp11/forwarding_references.cpp)。

## 进阶用法

**引用折叠决定最终类型。** `T` 推导为 `U&` 时，`T&&` 折叠为 `U&`；推导为 `U` 时保持 `U&&`。四条规则是：`& &`、`& &&`、`&& &` 都折叠为 `&`，只有 `&& &&` 折叠为 `&&`。

**左值推导带引用。** 传入 `std::string` 左值，`T` 是 `std::string&`；传入 `const std::string` 左值，`T` 是 `const std::string&`。这正是 `std::forward<T>` 判断值类别的依据。

**`std::forward` 不是无条件转换。** `std::move` 总是产生右值，`std::forward<T>` 只在 `T` 不是左值引用时才转换；转发引用必须搭配 `std::forward`，否则右值也会退化为左值。

**`auto&&` 也是转发引用。** range-for 与泛型 lambda 里的 `auto&&` 遵循同一推导规则，C++14 的泛型 lambda 让这种写法可以出现在局部函数对象中。

**`const T&&` 不是转发引用。** 转发引用要求 `T` 本身不被 cv 限定；`const T&&` 无法绑定左值实参，只是普通右值引用。

**也可以转发到构造函数。** `make_unique`、`emplace_back`、`std::make_shared` 都以模板参数包加 `std::forward` 的形式把参数原样交给目标构造函数。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                     | 用法                               | 为什么典型                                 |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ---------------------------------- | ------------------------------------------ |
| spdlog | [`include/spdlog/common.h#L383-L387`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/common.h#L383-L387)                  | `make_unique` 用 `Args&&` 完美转发 | C++14 之前自行实现，左值拷贝、右值移动     |
| drogon | [`lib/inc/drogon/HttpBinder.h#L231-L236`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/HttpBinder.h#L231-L236) | 构造函数用 `FUNCTION&&` 保存回调   | 任意可调用对象按值类别转发，避免多余拷贝   |
| mold   | [`lib/common.h#L95-L98`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/common.h#L95-L98)                                              | 输出代理用 `T&&` 转交给内部流      | 链式 `operator<<` 需要保留任意类型的值类别 |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖工厂函数、构造函数与运算符模板三种转发场景。

## 陷阱与边界

**转发引用只认模板推导。** `std::vector<T>&&`、`int&&` 都不是转发引用；必须是函数模板参数且形如 `T&&`，`T` 在该调用中被推导。

**同一个实参只能转发一次。** 右值被转发并移动后处于有效但未指定状态；循环或分支里重复 `std::forward` 同一对象，后一次通常读到空值。

**转发构造函数会劫持拷贝构造。** 类模板里写 `template <typename T> Wrapper(T&&)` 时，传入同类型的非 const 左值会优先匹配模板构造函数，而不是拷贝构造函数，需要 `enable_if` 或 concepts 约束排除自身类型。

**`std::forward` 需要显式模板实参。** 写成 `std::forward(value)` 无法推导，必须 `std::forward<T>(value)`；漏写模板实参通常直接编译失败。

**引用折叠对 `auto` 同样生效。** `auto&& x = expr;` 中 `x` 的类型随 `expr` 的值类别变化，误以为它总是右值引用会导致悬空引用或意外移动。

**MSVC 早期实现有差异。** VS2010（16.0）支持右值引用与引用折叠，但模板推导和 `std::forward` 在复杂参数包上的行为要到后续版本才稳定。

## 库联动

- **`<utility>` 的 `std::forward` 与 `std::move`**：转发引用负责推导，二者负责把值类别交给下一层。
- **`std::make_unique` / `std::make_shared`**：参数包加转发引用是工厂函数的标准形态。
- **`emplace_back` / `emplace`**：容器原位构造依赖完美转发把参数送到元素构造函数。
- **`std::function` 与回调包装**：构造时转发可调用对象，保存后不再保留原始值类别。
- **`auto&&` 与 range-for**：通用引用让循环变量按需绑定左值或右值。
- **spdlog / drogon / mold**：分别展示工厂函数、构造函数与运算符模板三种转发写法。

## 横向对比

| 方案                      | 右值保持 | 代码量 | 主要限制                           |
| ------------------------- | -------- | ------ | ---------------------------------- |
| 转发引用 + `std::forward` | 是       | 低     | 推导规则复杂，易与普通右值引用混淆 |
| 为左右值各写重载          | 是       | 高     | 参数个数增加后组合爆炸             |
| `const T&` 接收           | 否       | 低     | 右值也只能拷贝，无法移动           |
| 按值传递后 `std::move`    | 是       | 中     | 左值实参总要多一次拷贝             |

跨标准演进：C++11 通过 N2118 引入右值引用、引用折叠与 `std::forward`；C++14 的泛型 lambda 让 `auto&&` 进入局部函数对象；C++17 的类模板实参推导减少显式模板参数；C++20 的 concepts 可以把转发构造函数约束到非自身类型，缓解重载劫持。
