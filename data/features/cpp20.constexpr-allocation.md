---
id: cpp20.constexpr-allocation
title: constexpr 中的动态分配
standard: C++20
kind: language
category: [编译期计算, 内存管理]
difficulty: 4
status: final
tier: A
compilers: {gcc: "10", clang: "12", msvc: "19.28"}
tags: [constexpr, consteval, new, delete, constexpr string, 编译期容器]
alternatives: [固定大小 std::array, 手写编译期缓冲区, 模板元编程, 代码生成]
proposal: P0784R7
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "在 constexpr 函数里构造 std::string，并 reserve 计算好的输出长度"
    evidence: [ev-glaze-constexpr-string-escape-001]
  - id: c2
    statement: "用返回 std::string 的 constexpr 函数在编译期完成键名改写"
    evidence: [ev-glaze-constexpr-string-rename-002]
  - id: c3
    statement: "用 consteval 求出字符串长度，再填充固定大小数组保存结果"
    evidence: [ev-glaze-consteval-renamed-size-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

C++11 起 constexpr 函数可以有循环、分支和局部变量，但内存只能是固定大小的数组。编译期需要处理长度未知的字符串或元素个数不确定的序列时，只能靠模板递归、手写缓冲区或用外部脚本预先生成代码。`std::string`、`std::vector` 都被排除在常量求值之外。

C++20 允许常量求值期间进行动态分配，条件是所有内存在同一次常量求值的末尾之前释放。这样 constexpr 函数可以临时构造 `std::string` 或 `std::vector` 做中间计算，只要不把它们带出常量求值。标准提案为 P0784R7，库侧的 `constexpr std::string` 与 `constexpr std::vector` 随后由 P0980R1 和 P1004R2 补齐。

## 基础用法

constexpr 函数里 `new` 与 `delete` 必须由该函数自己配对，不能把分配结果返回出去。

<!-- compile -->

```cpp
#include <iostream>

constexpr int sum_squares(int n) {
  int *values = new int[n];
  for (int i = 0; i < n; ++i) {
    values[i] = i * i;
  }
  int sum = 0;
  for (int i = 0; i < n; ++i) {
    sum += values[i];
  }
  delete[] values;
  return sum;
}

int main() {
  constexpr int compile_time = sum_squares(4);
  const int run_time = sum_squares(5);

  std::cout << compile_time << ' ' << run_time << '\n';
}
```

输出 `14 30`。`compile_time` 在编译期求值，`run_time` 因为实参来自运行期而走普通执行路径；同一个函数两种求值都成立。完整版本见 [`examples/cpp20/constexpr_allocation.cpp`](../../examples/cpp20/constexpr_allocation.cpp)。

分配不要求是常量大小。`n` 可以是运行期实参，只有当整个调用出现在常量表达式中时才进入求值器，此时 `new` 返回常量求值器管理的存储。

## 进阶用法

常量求值期间的分配遵循一条核心约束：到常量求值结束时，所有分配都必须已经释放。返回指向已释放内存的指针、把 `new` 出来的数组存进外部变量，都会让整个常量表达式失败。

`consteval` 与 constexpr 分配配合时效果最好。`consteval` 函数保证只在编译期求值，内部可以放心使用动态分配；结果通常被复制到 `std::array` 或其它能长期存在的存储中，避免把瞬态分配的地址带出去。

标准库容器在 C++20 中也变成了 constexpr。`constexpr std::string` 允许在常量求值中拼接、替换和查找，`constexpr std::vector` 允许在常量求值中 `push_back`。它们的编译期支持门槛比语言特性更高：GCC 12、Clang 15、MSVC 19.29 才完整提供。

常量求值器对分配次数和总大小有实现定义的上限。超出限制会报“超过最大步数”或内存配额类的错误，这种错误通常随编译器版本变化，不应写进可移植代码的假设里。

`constexpr` 函数在运行期被调用时走普通堆分配路径，没有额外机制保证性能。编译期求值只是其中一种调用方式。

## 真实项目证据

| 项目  | 位置                                                                                                                                                                                         | 用法                                                                  | 为什么典型                                       |
| ----- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------- | ------------------------------------------------ |
| glaze | [`include/glaze/json/escape_unicode.hpp#L125-L129`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/json/escape_unicode.hpp#L125-L129)     | `constexpr std::string escape_json_string` 中构造输出字符串并预留容量 | 序列化库在编译期准备转义结果，运行期直接读取     |
| glaze | [`include/glaze/util/key_transformers.hpp#L249-L255`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/util/key_transformers.hpp#L249-L255) | `constexpr std::string rename_key` 把 snake_case 键名转成 camelCase   | 编译期改写键名，避免运行期字符串处理             |
| glaze | [`include/glaze/reflection/get_name.hpp#L321-L333`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/reflection/get_name.hpp#L321-L333)     | 用 consteval 求出的长度填充 `std::array<char, N + 1>`                 | 瞬态分配只用于求长度，最终结果落在固定大小数组中 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。glaze 还用宏检查 `_GLIBCXX_USE_CXX11_ABI` 等条件，在不支持 constexpr 字符串的旧 ABI 上关闭这条路径，说明库作者需要为编译期容器准备回退方案。

## 陷阱与边界

**分配必须自己释放。** constexpr 函数中 `new` 出来的内存若未在常量求值结束前释放，常量表达式判定失败，报错位置往往指向整个调用而不是泄漏点。

**不能把分配结果带出去。** 返回指向瞬态分配的指针、把它存进静态变量，或让 `std::string` 持有已释放的存储，都会使常量求值无效。需要长期保存时，把内容复制到 `std::array` 等定长存储。

**运行期调用不享受编译期检查。** `constexpr` 函数被运行期调用时就是普通函数，上面的泄漏同样会造成真实的堆泄漏。

**求值器有资源上限。** 分配次数、总字节数和步骤数都存在实现定义的限制，大型常量表达式可能直接编译失败。

**库支持晚于语言支持。** 语言层面 GCC 10、Clang 12、MSVC 19.28 即可使用；但要构造 `std::string`、`std::vector`，需要 GCC 12、Clang 15、MSVC 19.29。混用会看到“不是常量表达式”一类错误。

**`constexpr` 不保证编译期求值。** 只有出现在常量表达式语境（`constexpr` 变量、模板实参、`static_assert`）或调用了 `consteval` 函数时，求值器才会介入。

## 库联动

- **glaze**：用 constexpr 字符串完成 JSON 键名转换与转义，反射与序列化在编译期定型。
- **`std::array`**：编译期计算的结果通常落进定长数组，避免把瞬态分配的地址带出常量求值。
- **模板元编程**：过去靠递归模板拼接类型列表的场景，可以改写成 constexpr 函数中的循环。
- **代码生成**：原本用外部脚本预生成查找表的项目，可以把生成逻辑前移到 constexpr 函数。

## 横向对比

| 方案                  | 能否处理动态长度 | 编译期可调试性 | 运行期开销       | 主要限制                     |
| --------------------- | ---------------- | -------------- | ---------------- | ---------------------------- |
| constexpr 动态分配    | 是               | 一般           | 无（结果已定型） | 分配必须在同一求值内释放     |
| 固定大小 `std::array` | 否               | 好             | 无               | 需要提前知道上界             |
| 模板元编程            | 部分             | 差             | 无               | 表达复杂逻辑非常笨重         |
| 外部代码生成          | 是               | 好             | 无               | 增加构建步骤，源与代码易脱节 |

跨标准演进：C++20 通过 P0784R7 允许常量求值中的动态分配，并由 P0980R1、P1004R2 把 `std::string` 与 `std::vector` 变成 constexpr 容器；C++23 继续放宽了部分 constexpr 限制，但“分配不得逃逸出求值”这条边界始终不变。
