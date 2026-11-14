---
id: cpp17.as-const
title: std::as_const
standard: C++17
kind: library
category: [类型转换, 类型工具]
difficulty: 2
status: final
tier: A
compilers: {gcc: "7.1", clang: "3.8", msvc: "19.0"}
tags: [as_const, const, 引用, 重载决议, 类型推导]
alternatives: [static_cast<const T&>, std::cref, const 引用变量, std::add_const_t]
proposal: P0007R1
last_verified: "2026-11-14"
claims:
  - id: c1
    statement: "非 const 成员函数用 std::as_const 调用同名 const 重载以复用实现"
    evidence: [ev-hyprland-as-const-overload-001]
  - id: c2
    statement: "把结果容器以 std::as_const 的 const 引用传给完成回调，避免复制"
    evidence: [ev-stdexec-as-const-completion-002]
  - id: c3
    statement: "用 std::as_const 取得容器的 const 左值，再构造只读 span"
    evidence: [ev-simdutf-as-const-span-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

同一个对象常常需要以只读身份使用：在非 `const` 成员函数里复用 `const` 重载、把容器传给只接受 `const` 引用的接口、或者让 `range-for` 不复制元素。手写 `static_cast<const T&>(x)` 能达到目的，但出现在调用点会掩盖意图；`std::cref` 返回的是 `reference_wrapper`，类型已经变了，只适合模板适配；先声明一个 `const auto&` 又会在需要表达式的地方多出一个名字。

`std::as_const` 提供一个精确表达“把这个左值当作 `const` 左值”的函数。它返回 `const T&`，类型与原对象一致，只是多了 `const`，可以直接嵌在调用表达式里。P0007R1 把它加入 `<utility>`，同时删除了接受右值的重载，避免把临时对象的引用留到语句之外。

## 基础用法

`std::as_const(x)` 接受一个左值，返回 `const T&`；传入右值时对应的重载被删除，编译期直接报错。

<!-- compile -->

```cpp
#include <iostream>
#include <string>
#include <utility>

int main() {
  std::string text = "atlas";
  const std::string& view = std::as_const(text);
  std::cout << view.size() << '\n';
}
```

输出：

```text
5
```

完整版本见 [`examples/cpp17/as_const.cpp`](../../examples/cpp17/as_const.cpp)，六个场景的输出为：

```text
[1] as_const is const int& = true
[2] overload: non-const = 1, const = 2
[3] same address = true
[4] size non-const = 10, const = 20
[5] sum = 6, size = 3
[6] string is const = true
```

场景 1 用 `static_assert` 确认返回类型就是 `const int&`。场景 2 让重载决议在非 `const` 与 `const` 两个版本之间切换。场景 3 比较地址，确认没有产生副本。场景 4 展示成员函数的 `const` 重载选择。场景 5 在 `range-for` 中以只读方式遍历容器。场景 6 说明 `auto&&` 推导后仍保留 `const` 限定。

## 进阶用法

**复用 `const` 重载。** 当同一逻辑同时需要 `const` 与非 `const` 版本时，把实现写在 `const` 成员函数里，非 `const` 版本用 `std::as_const(*this)` 委派过去。Hyprland 的 `CWindow::layoutTarget()` 就是这种写法，既避免复制 `*this`，也不必手写 `const_cast`。

**避免热路径复制。** 只读接口只接受 `const` 引用时，直接传容器会先经过一次隐式转换，容易在重载或模板推导中产生副本。先用 `std::as_const` 明确成 `const` 左值，可以稳定选中只读版本。stdexec 在启动操作状态时把结果容器以 `std::as_const(*_results_)` 传给完成回调，调用的是 `const` 成员函数。

**构造只读视图。** `std::span` 等视图的构造依赖实参的 `const` 性。simdutf 的测试同时传入可变容器、`std::span`、`std::as_const` 的容器与右值，覆盖视图的各个构造分支。

**它不加顶层 `const` 到指针目标。** `std::as_const(ptr)` 的类型是 `T* const&`，被 `const` 化的是指针本身，不是 `*ptr`。要禁止通过指针修改目标，需要 `std::as_const(*ptr)` 或 `const T*`。

**只接受左值。** 标准删除了 `const T&&` 重载，`std::as_const(std::string{"x"})` 不能编译。这条限制防止返回的引用指向即将销毁的临时对象。

## 真实项目证据

| 项目           | 位置                                                                                                                                                                                | 用法                                                             | 为什么典型                                                |
| -------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------- | --------------------------------------------------------- |
| Hyprland       | [`src/desktop/view/window/Window.cpp#L1938-L1940`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/desktop/view/window/Window.cpp#L1938-L1940) | 非 `const` 成员函数用 `std::as_const(*this)` 委派给 `const` 重载 | 同一逻辑只写一次，`as_const` 代替 `const_cast` 且语义明确 |
| NVIDIA/stdexec | [`include/exec/fork_join.hpp#L74-L77`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/fork_join.hpp#L74-L77)                          | 结果容器以 `std::as_const` 传给完成回调，避免复制                | 启动路径只读结果，`as_const` 选定 `const` 成员函数        |
| simdutf        | [`tests/span_tests.cpp#L17-L23`](https://github.com/simdutf/simdutf/blob/59589cd3df6823cbfdbd253cde3377e7c3e1fe9b/tests/span_tests.cpp#L17-L23)                                     | 用 `std::as_const` 得到 `const` 容器，再构造 `std::span`         | 视图构造依赖实参 `const` 性，`as_const` 明确只读来源      |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。两处来自生产路径，一处来自视图测试，覆盖重载复用、回调传参与只读视图三种用法。

## 陷阱与边界

**它只给对象加 `const`，不深入指针或容器元素。** `std::as_const(ptr)` 得到的是 `T* const&`；`std::as_const(vec)` 得到 `const std::vector<T>&`，元素仍可通过其他非 `const` 别名修改。别把它当成深层次的不可变保证。

**不能绑定右值。** 标准用删除的 `const T&&` 重载阻止临时对象，传入右值会编译失败。需要延长临时对象生命周期时，直接声明 `const auto&`。

**`mutable` 成员仍可修改。** `const` 只限制通过该引用调用非 `const` 成员，`mutable` 成员与 `const_cast` 不受影响。

**重载语义可能不同。** `const` 与非 `const` 重载可以返回不同类型或走完全不同的实现；`as_const` 只负责选择，不保证两个版本行为一致。

**返回值是引用，注意生命周期。** `as_const` 不复制，引用的仍是原对象。若原对象先于引用销毁，后续访问就是悬垂引用。

**旧工具链没有它。** GCC 7.1、Clang 3.8、MSVC 19.0 Update 2 起提供。更早版本只能写 `static_cast<const T&>(x)`。

## 库联动

- **`std::cref` / `std::reference_wrapper`**：同样提供只读引用，但类型是包装器；`as_const` 保持原类型，适合普通函数调用。
- **`std::move`**：一个转移所有权、一个增加 `const`，都只改变表达式的值类别或限定符，不复制对象。
- **`std::span` / `std::string_view`**：构造只读视图时常配合 `as_const` 明确来源的 `const` 性。
- **`std::is_const` / `decltype`**：用 `as_const` 后的类型做编译期判断，验证重载与推导结果。

## 横向对比

| 方案                    | 返回类型                     | 接受右值 | 保持原类型 | 主要限制                   |
| ----------------------- | ---------------------------- | -------- | ---------- | -------------------------- |
| `std::as_const`         | `const T&`                   | 否       | 是         | C++17 起；不深入指针目标   |
| `static_cast<const T&>` | `const T&`                   | 是       | 是         | 可绑定临时对象，易悬垂     |
| `std::cref`             | `reference_wrapper<const T>` | 是       | 否         | 包装器类型，需模板适配     |
| `const auto&`           | `const T&`                   | 是       | 是         | 需要额外命名，不能直接内联 |

`std::as_const` 由 P0007R1 进入 C++17，把“把这个左值当作 `const` 用”从一次显式转换变成一个表意清楚的函数调用。它不复制、不深入指针、也不接受右值，边界刻意收得很窄，正适合重载复用与只读传参这类高频但容易写错的场景。
