---
id: cpp17.size-data-empty
title: std::size / std::data / std::empty
standard: C++17
kind: library
category: [容器, 范围]
difficulty: 1
status: final
tier: A
compilers: {gcc: "6", clang: "3.6", msvc: "19.0"}
tags: [size, data, empty, 非成员函数, 容器长度]
alternatives: [成员 size(), 成员 data(), sizeof 除法, end 减 begin, std::span]
proposal: N4280
last_verified: "2026-11-18"
claims:
  - id: c1
    statement: "解析 BSON 输入时用 std::data 与 std::size 取得缓冲区首指针和长度"
    evidence: [ev-glaze-size-data-bson-001]
  - id: c2
    statement: "崩溃报告器对静态 C 数组用 std::size 取元素个数"
    evidence: [ev-hyprland-size-array-002]
  - id: c3
    statement: "图算法把固定大小数组成员交给回调前用 std::size 计算元素个数"
    evidence: [ev-abseil-size-stack-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

泛型代码要拿到容器长度，过去只能调用成员 `size()`；遇到裸数组就得写 `sizeof(arr) / sizeof(arr[0])`，数组一旦退化成指针，这个式子会静默给出错误结果。取首元素地址同样分裂成成员 `data()`、`&vec[0]` 和 `arr` 三种写法，判断是否为空则要组合 `size() == 0`、`begin() == end()` 或数组长度比较。

N4280 把这三类操作统一成非成员函数 `std::size`、`std::data`、`std::empty`。它们对标准容器、裸数组和 `std::initializer_list` 都可用，写法一致，也不会让数组退化。C++17 起这三个函数定义在 `<iterator>` 中。

## 基础用法

三个函数都接受容器或数组，返回长度、首元素指针和是否为空。

<!-- compile -->

```cpp
#include <iostream>
#include <iterator>
#include <vector>

int main() {
  int raw[] = {10, 20, 30, 40};
  std::vector<int> values{1, 2, 3};

  std::cout << std::size(raw) << ' ' << std::size(values) << '\n';
  std::cout << *std::data(values) << ' ' << std::empty(values) << '\n';
}
```

输出 `4 3` 和 `1 0`。完整版本见 [`examples/cpp17/size_data_empty.cpp`](../../examples/cpp17/size_data_empty.cpp)，六个场景的输出为：

```text
[1] raw size = 4
[2] vector size = 3
[3] template size = 5
[4] first = 10
[5] vector data = 1
[6] empty raw = 0, empty none = 1
```

场景 1、2 分别对裸数组和 `std::vector` 取长度。场景 3 在模板函数里调用 `std::size`，同一个函数体同时接受 `std::array` 等容器。场景 4、5 用 `std::data` 取首元素地址。场景 6 对比非空数组与空 vector 的 `std::empty` 结果。

`std::size` 返回 `size_type`（通常为 `std::size_t`）；`std::data` 返回指向首元素的指针，空容器时行为由容器的 `data()` 决定；`std::empty` 返回 `bool`。三者都是 `constexpr`，可用于常量表达式。

## 进阶用法

**模板里统一取长度。** 写接受任意容器的函数时，把参数声明为 `const Container&`，函数体用 `std::size(container)` 即可。这个签名同样接受裸数组：`Container` 会推导成 `int[4]` 这样的数组类型，按引用传入不会退化，`std::size` 能看到真实长度。只有按值传参才会把数组退化成指针。

**按字节读取缓冲区。** BSON、协议帧或二进制文件的解析器常需要“首地址 + 长度”这一对值。`std::data(input)` 和 `std::size(input)` 可以同时作用于 `vector`、`array`、`span` 和裸数组，读取端不必关心输入容器的具体类型。

**先判空再取首元素。** `std::empty` 对容器调用成员 `empty()`，对数组比较长度是否为零；它比 `std::size(c) == 0` 更能表达意图，也避免自定义容器上多余的函数调用。

**与指针算法配合。** 拿到 `std::data` 返回的指针后，可以直接做 `it + std::size(input)` 得到尾后指针，再交给需要 `[first, last)` 区间的算法或第三方接口。

**避免 `sizeof` 除法。** `sizeof(arr) / sizeof(arr[0])` 在数组退化成函数参数后会算成指针大小之比，得到错误长度；这不是编译错误，只有开启警告时 GCC 8+ 与 Clang 才会用 `-Wsizeof-pointer-div` 提示；`std::size` 只在真正能看到数组类型时接受裸数组，失效场景会直接编译失败。

## 真实项目证据

| 项目               | 位置                                                                                                                                                                                                  | 用法                                               | 为什么典型                                                   |
| ------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------- | ------------------------------------------------------------ |
| stephenberry/glaze | [`include/glaze/bson/bson_to_json.hpp#L357-L358`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/bson/bson_to_json.hpp#L357-L358)                  | 用 `std::data` 与 `std::size` 取得输入首指针和长度 | 非成员接口对多种输入容器一视同仁，反序列化端无需区分容器类型 |
| hyprwm/Hyprland    | [`src/debug/crash/CrashReporter.cpp#L42-L42`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/src/debug/crash/CrashReporter.cpp#L42-L42)                             | 对静态 C 数组用 `std::size` 取长度并取模           | 裸数组场景省略 `sizeof` 除法，信号安全代码保持简单           |
| abseil/abseil-cpp  | [`absl/synchronization/internal/graphcycles.cc#L701-L702`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/synchronization/internal/graphcycles.cc#L701-L702) | 把 C 数组成员交给回溯函数时计算元素个数            | 图算法内部对固定大小数组求长度，返回值类型明确为 `size_t`    |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

**裸数组必须以引用传入模板。** `template <class T> void f(T arr)` 里的 `T` 会退化成指针，`std::size(arr)` 无法编译；应写成 `template <class T, std::size_t N> void f(T (&arr)[N])`，或直接接受 `std::span`。

**要求有对应成员函数，或是数组与 `initializer_list`。** 标准容器之外，自定义类型只要提供对应的成员函数（`size()`、`data()`、`empty()`）也会被接受；没有这些成员的类型不会自动适配。

**`std::data` 对 `std::vector<bool>` 不可用。** `vector<bool>` 是位压缩特化，没有 `data()` 成员，非成员版本也随之不可用；需要原始字节时先转换到其他容器。

**返回类型是 `size_t`，与有符号整数混用会告警。** 和 `int` 下标比较时注意 `-Wsign-compare`；需要有符号长度时用 C++20 的 `std::ssize`。

**`std::empty` 判断的是逻辑长度。** 对 `std::string`、`std::vector` 等容器，它调用成员 `empty()`，与容量无关；`capacity() == 0` 不能替代它。

**它们不返回范围。** 三个函数只给出长度、指针或布尔值，不构成迭代器区间；需要遍历时仍要用 `begin()` / `end()` 或范围 `for`。

**头文件。** 标准保证 `<iterator>` 提供这三个函数，`<array>`、`<deque>`、`<forward_list>`、`<list>`、`<map>`、`<regex>`、`<set>`、`<string>`、`<string_view>`、`<unordered_map>`、`<unordered_set>`、`<vector>`（C++20 起还有 `<span>`）也保证提供。只处理裸数组、没有包含任何容器头时，要显式包含 `<iterator>`。

## 库联动

- **`std::begin` / `std::end`**：C++11 起提供的非成员迭代器访问，与 `size` / `data` / `empty` 同属泛型访问设施。
- **`std::span`**：C++20 的视图，可直接由 `std::data(c)` 与 `std::size(c)` 构造，把“指针 + 长度”封装成带边界的对象。
- **`std::ssize`**：C++20 补充的有符号长度版本，适合与 `int` 索引比较。
- **`std::vector` / `std::array` / `std::string`**：都提供同名成员函数，非成员版本对它们透明转发。
- **`std::copy` / `std::memcpy`**：读取缓冲区时，`std::data` 与 `std::size` 正好给出源区间。

## 横向对比

| 方案                           | 适用对象                       | 裸数组 | 返回值                 | 主要限制                         |
| ------------------------------ | ------------------------------ | ------ | ---------------------- | -------------------------------- |
| `std::size` / `data` / `empty` | 容器、数组、`initializer_list` | 支持   | `size_t`、指针、`bool` | 要求类型有对应成员或为数组       |
| 成员 `size()` / `data()`       | 标准容器                       | 不支持 | 由容器定义             | 泛型代码需要区分容器与数组       |
| `sizeof(arr) / sizeof(*arr)`   | 裸数组                         | 支持   | `size_t`               | 数组退化为指针后算错，仅告警提示 |
| `end() - begin()`              | 容器                           | 不支持 | 差值类型               | 需要有效迭代器，空容器边界需注意 |
| `std::ssize`                   | 容器、数组                     | 支持   | 有符号整数             | C++20 起可用                     |

N4280 把长度、首地址与空判断收敛到同一组非成员函数。它不改变容器的存储或复杂度，但让泛型代码在容器与裸数组之间使用同一套写法。
