---
id: cpp20.bit-operations
title: 位操作函数（std::popcount 等）
standard: C++20
kind: library
category: [性能, 算法]
difficulty: 3
status: final
tier: A
compilers: {gcc: "9", clang: "9", msvc: "19.28"}
tags: [bit, popcount, countl_zero, countr_zero, rotl]
alternatives: [__builtin_popcount, 手写循环, absl/numeric/bits, Boost.Core, 编译器 intrinsics]
proposal: P0553R4
last_verified: "2026-11-12"
claims:
  - id: c1
    statement: "用 std::countr_zero 把 2 的幂对齐值换算成移位位数"
    evidence: [ev-mold-bitops-countr-zero-001]
  - id: c2
    statement: "编译期用 std::popcount 断言缓冲区对齐是 2 的幂"
    evidence: [ev-stdexec-bitops-popcount-002]
  - id: c3
    statement: "估算十进制位数时用 std::countl_zero 取得最高有效位位置"
    evidence: [ev-nghttp2-bitops-countl-zero-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

位级操作长期依赖编译器内建函数：`__builtin_popcount`、`__builtin_clz`、`__builtin_ctz` 在不同编译器上名字不同，参数宽度要自己区分，返回值语义也各有细节。可移植代码只能退回手写循环，性能和可读性都受影响；包装内建函数又要维护一层宏和特性检测。

C++20 的 `<bit>` 头文件把这组操作标准化：`std::popcount` 数置位个数，`std::countl_zero` / `std::countr_zero` 数前导与尾随零，`std::countl_one` / `std::countr_one` 数前导与尾随一，`std::rotl` / `std::rotr` 做循环移位。它们都是 `constexpr`，只接受无符号整数类型，语义在所有平台上一致。

## 基础用法

这些函数按值接收无符号整数，返回 `int`（循环移位返回与输入相同的类型）。`<bit>` 同时提供 `std::bit_cast`（C++20 的另一项特性）。

<!-- compile -->

```cpp
#include <bit>
#include <iostream>

int main() {
  unsigned value = 0b10110100u;
  std::cout << std::popcount(value) << ' ' << std::countr_zero(value) << '\n';
}
```

输出：

```text
4 2
```

完整版本见 [`examples/cpp20/bit_operations.cpp`](../../examples/cpp20/bit_operations.cpp)，六个场景的输出为：

```text
[1] popcount = 4
[2] trailing = 4, leading = 27
[3] trailing ones = 3, leading ones = 0
[4] rotl = 2880, round trip = 1
[5] alignment = 4096, shift = 12, recovered = 4096
[6] width = 10, highest = 9
```

场景 1 数出掩码中 4 个置位位。场景 2 对 `0x10` 分别取尾随零和前导零，得到 4 与 27。场景 3 对低三位全为 1 的字节取尾随一和前导一。场景 4 左旋 4 位再右旋 4 位，验证可逆。场景 5 用尾随零个数从对齐值还原移位量，正是链接器与分配器的常见写法。场景 6 用前导零从位宽推出生效位宽与最高位下标。

## 进阶用法

**约束到无符号整数。** 泛型代码可以用 `std::unsigned_integral` 约束模板参数，避免有符号类型带来的符号位歧义。传给这些函数的实参必须是无符号整数类型，`int`、`bool` 或自定义类型都不满足要求。

**编译期断言。** 函数是 `constexpr`，可以直接放进 `static_assert`。例如用 `std::popcount(x) == 1` 验证某个模板参数是 2 的幂，失败信息会在实例化时给出。

**位宽来自 `std::numeric_limits`。** `std::countl_zero` 的返回值依赖类型宽度；计算有效位宽时用 `std::numeric_limits<T>::digits - std::countl_zero(x)`，不要硬编码 32 或 64。

**循环移位的计数。** `std::rotl` / `std::rotr` 接收 `int` 计数，并按类型位宽归约。旋转常用于哈希、校验和与加密实现，替代“移位加或”的手写组合。

**Power-of-2 是另一组接口。** `std::has_single_bit`、`std::bit_ceil`、`std::bit_floor`、`std::bit_width` 来自 P0556R3，与本页的计数、旋转函数一起构成 `<bit>`，但属于独立提案与另一组语义；需要判断 2 的幂时优先用 `has_single_bit`，不要用 `popcount(x) == 1` 漏掉 0。

## 真实项目证据

| 项目           | 位置                                                                                                                                                                           | 用法                                           | 为什么典型                              |
| -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ---------------------------------------------- | --------------------------------------- |
| mold           | [`src/input-sections.cc#L9-L13`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/input-sections.cc#L9-L13)                                    | 用 `std::countr_zero` 把对齐值换算成移位量     | 对齐值总是 2 的幂，尾随零个数就是移位量 |
| NVIDIA/stdexec | [`include/stdexec/__detail/__any.hpp#L735-L735`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__any.hpp#L735-L735) | 编译期用 `std::popcount` 断言对齐是 2 的幂     | `constexpr` 让断言直接写在模板定义里    |
| nghttp2        | [`src/util.h#L734-L737`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/util.h#L734-L737)                                                | 用 `std::countl_zero` 取得最高有效位后估算位数 | 位数上界由二进制位宽与最高位位置决定    |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，分别对应链接器对齐、模板静态断言和协议工具函数。

## 陷阱与边界

**只接受无符号整数。** 有符号类型、`char`（实现定义了是否有符号）与浮点类型都不满足约束。需要处理有符号输入时先转换到对应的无符号类型，并明确负数语义。

**零值不是错误。** `std::countl_zero(0)` 与 `std::countr_zero(0)` 都返回类型位宽，不是 0，也不会触发未定义行为。依赖“零没有前缀”的代码要单独判断输入是否为零。

**`popcount` 返回 `int`。** 结果最大等于类型位宽，放进更窄的变量前要确认范围。`countl_*` / `countr_*` 同样返回 `int`。

**宽度随类型变化。** 同一个值在 `std::uint32_t` 与 `std::uint64_t` 上的前导零数量不同。模板代码要按 `T` 的实际位宽计算，不能假设 32 位。

**循环移位不是普通移位。** `std::rotl` 会把移出的位补回另一端，`<<` 会丢弃。需要饱和或清零语义时应使用普通移位并显式处理边界。

**旧工具链没有这些函数。** GCC 9、Clang 9、MSVC 19.28 起才完整提供。更早的编译器需要内建函数或 Abseil 的 `absl::bits` 兼容层；跨标准库使用时先检测 `<bit>` 与具体符号是否可用。

## 库联动

- **`std::bit_cast`**：与位操作同属 `<bit>`，用于在不破坏别名规则的前提下重解释位表示。
- **`std::numeric_limits`**：提供 `digits`，与 `countl_zero` 配合计算有效位宽。
- **`absl/numeric/bits`**：Abseil 提供兼容实现，并在可用时转发到 `std` 版本。
- **哈希与校验和**：旋转、置位计数是混合函数与校验算法的基本构件。
- **C++23 `std::byteswap`**：处理字节序的补充接口；位计数与旋转不涉及字节序。

## 横向对比

| 方案             | 可移植 | `constexpr` | 性能               | 主要限制                         |
| ---------------- | ------ | ----------- | ------------------ | -------------------------------- |
| `<bit>` 标准函数 | 是     | 是          | 通常编译为单条指令 | C++20 起；仅无符号整数           |
| 编译器内建函数   | 否     | 部分        | 高                 | 名称与语义随编译器变化           |
| 手写循环         | 是     | 是          | 低                 | 分支多，编译器未必能识别为位操作 |
| `absl::bits`     | 是     | 是          | 高                 | 引入 Abseil 依赖，作为兼容层使用 |

`<bit>` 的位操作函数由 P0553R4 进入 C++20，把过去散落在内建函数和第三方库里的计数、旋转操作统一成标准接口。它们适合底层数据结构、序列化、压缩与哈希代码；普通业务逻辑里若只是为了判断 2 的幂，优先考虑同头文件的 `std::has_single_bit`。
