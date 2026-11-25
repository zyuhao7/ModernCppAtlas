---
id: cpp20.bit-power
title: 2 的幂运算（std::bit_ceil 等）
standard: C++20
kind: library
category: [性能, 算法]
difficulty: 2
status: final
tier: A
compilers: {gcc: "9", clang: "9", msvc: "19.28"}
tags: [bit_ceil, bit_floor, bit_width, has_single_bit, 2 的幂]
alternatives: [手写移位循环, __builtin_clz, absl/numeric/bits, Boost.Core]
proposal: P0556R3
last_verified: "2026-11-24"
claims:
  - id: c1
    statement: "用 has_single_bit 判断枚举值是否为 2 的幂，并排除零值"
    evidence: [ev-glaze-bitpower-has-single-bit-001]
  - id: c2
    statement: "用 bit_ceil 把请求容量向上取整到 2 的幂，以便用掩码取模"
    evidence: [ev-stdexec-bitpower-ceil-lifo-002]
  - id: c3
    statement: "特性宏可用时把库内 2 的幂函数别名到标准实现"
    evidence: [ev-abseil-bitpower-alias-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

“向上/向下取整到 2 的幂”“判断一个数是不是 2 的幂”“一个数需要几位二进制表示”是底层代码里的高频需求。过去这些操作要么手写移位或循环，要么依赖 `__builtin_clz` 之类的编译器内建函数；不同平台的名字、位宽和边界语义不一致，稍不留神就会在零值或超宽类型上出错。

C++20 的 `<bit>` 头文件统一了这组“2 的幂运算”：`std::has_single_bit` 判断是不是 2 的幂，`std::bit_ceil` 向上取整到 2 的幂，`std::bit_floor` 向下取整到 2 的幂，`std::bit_width` 返回表示该值所需的最小位数。它们都是 `constexpr`，只接受无符号整数类型，语义在全平台一致。

## 基础用法

这四个函数按值接收无符号整数。`has_single_bit` 返回 `bool`，其余三个返回与输入相同的无符号类型（`bit_width` 的返回值语义上更适合按 `int` 理解）。

<!-- compile -->

```cpp
#include <bit>
#include <cstdint>
#include <iostream>

int main() {
  const std::uint32_t value = 9;
  std::cout << std::has_single_bit(value) << ' ' << std::bit_ceil(value) << ' '
            << std::bit_floor(value) << ' ' << std::bit_width(value) << '\n';
}
```

完整示例见 [`examples/cpp20/bit_power.cpp`](../../examples/cpp20/bit_power.cpp)，覆盖六种场景，输出为：

```text
[1] has_single_bit(8) = 1, has_single_bit(9) = 0
[2] bit_ceil(9) = 16, bit_floor(9) = 8
[3] bit_width(8) = 4, bit_width(9) = 4
[4] lowest power-of-2 >= 44 = 64, highest <= 44 = 32
[5] table capacity for 5 slots = 8
[6] bit_ceil(0) = 1, bit_floor(0) = 0, bit_width(0) = 0
```

场景 1 区分 8（2 的幂）与 9（不是）。场景 2 对 9 向上取到 16、向下取到 8。场景 3 说明 8 和 9 都需要 4 位。场景 4 对 44 做上下取整。场景 5 演示用 `bit_ceil(5)` 得到 8，为哈希表或环形缓冲区选容量。场景 6 覆盖零值边界，`bit_ceil(0)` 为 1、`bit_floor(0)` 为 0、`bit_width(0)` 为 0。

## 进阶用法

**用 `bit_ceil` 选定掩码取模的容量。** 无锁队列、环形缓冲区和开放寻址哈希表常要求容量为 2 的幂，从而用 `index & (capacity - 1)` 代替取模。把请求的容量交给 `std::bit_ceil`，得到满足约束的最小值，比手写移位循环更清楚也更不容易越界。

**`has_single_bit` 天然排除零。** 判断标志位枚举时，`v != 0 && std::has_single_bit(v)` 一次覆盖“非零且恰好一个位”。`std::has_single_bit(0)` 返回 `false`，因此可以省掉一个显式的零值分支；不要用 `popcount(v) == 1` 再补判零。

**`bit_width` 估算位数上界。** 计算十进制或二进制输出缓冲、推导分桶数量时，`bit_width(n)` 直接给出所需位数，比 `digits - countl_zero(n)` 更直观。注意 `bit_width(0)` 返回 0，不是 1。

**编译期可用。** 四个函数都是 `constexpr`，可放进 `static_assert` 验证模板参数，或用于推导数组大小、缓冲容量等常量。

## 真实项目证据

| 项目               | 固定源码                                                                                                                                                                                 | 用法                                            | 为什么典型                                     |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------- | ---------------------------------------------- |
| stephenberry/glaze | [`include/glaze/core/reflect.hpp#L1143-L1149`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/core/reflect.hpp#L1143-L1149)           | 用 `has_single_bit` 识别标志位枚举并取位位置    | `has_single_bit(0)` 为假，一次判断同时排除零值 |
| NVIDIA/stdexec     | [`include/exec/detail/bwos_lifo_queue.hpp#L199-L205`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/detail/bwos_lifo_queue.hpp#L199-L205) | 用 `bit_ceil` 把块数取整到 2 的幂以支持掩码取模 | 无锁队列要求容量为 2 的幂，`mask_` 依赖该前提  |
| abseil/abseil-cpp  | [`absl/numeric/bits.h#L138-L143`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/numeric/bits.h#L138-L143)                                      | 特性宏可用时把 `absl` 版本别名到 `std` 版本     | 库保留兼容回退，同时把标准实现作为首选         |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **只接受无符号整数。** 有符号类型、`char`（有无符号由实现定义）和浮点类型都不满足约束。需要处理有符号输入时先转换到对应的无符号类型。
- **零值语义各不相同。** `bit_ceil(0)` 返回 1，`bit_floor(0)` 返回 0，`bit_width(0)` 返回 0，`has_single_bit(0)` 返回 `false`。依赖“零是边界”的代码要逐项确认，不能假设统一返回 0。
- **`bit_ceil` 溢出是未定义行为。** 当输入大于该类型能表示的最大 2 的幂时，向上取整的结果无法表示，标准规定此时行为未定义（更早的提案版本曾返回 0）。对可能接近上限的输入，应在调用前显式检查范围。
- **`bit_width` 不是“置位个数”。** 它是最高有效位的下标加一，等于二进制表示长度，和 `popcount` 语义不同。
- **与 `<bit>` 的计数/旋转函数不是同一页。** `popcount`、`countl_zero`、`rotl` 等属于 [位操作函数](cpp20.bit-operations.md)（P0553R4）；本页的四个函数来自 P0556R3，判断“2 的幂”应优先用这组而不是 `popcount == 1`。
- **旧工具链缺失。** GCC 9、Clang 9、MSVC 19.28 起才完整提供；更早的编译器需要内建函数或 `absl/numeric/bits` 兼容层。

## 库联动

- [位操作函数（std::popcount 等）](cpp20.bit-operations.md) 与本页同属 `<bit>`，但来自不同提案、语义不同。
- **`absl/numeric/bits`**：Abseil 提供兼容实现，并在可用时转发到 `std` 版本。
- **`std::bit_cast`**：同头文件的位重解释工具，常与位宽计算配合。
- **分配器与哈希表**：容量取整到 2 的幂是掩码取模实现的基础。
- **`std::countr_zero`**：拿到 2 的幂后再取尾随零个数，即得到对应的移位量或位下标。

## 横向对比

| 方案                | 可移植 | `constexpr` | 处理零值   | 主要限制                     |
| ------------------- | ------ | ----------- | ---------- | ---------------------------- |
| `<bit>` 2 的幂函数  | 是     | 是          | 有定义     | C++20 起；仅无符号整数       |
| 手写移位循环        | 是     | 是          | 自行处理   | 冗长，边界易错               |
| `__builtin_clz`     | 否     | 部分        | 零值未定义 | 名称与位宽随编译器变化       |
| `absl/numeric/bits` | 是     | 是          | 有定义     | 引入 Abseil 依赖，作为兼容层 |

2 的幂运算由 P0556R3 进入 C++20，把散落在内建函数和第三方库里的取整、判幂操作统一成标准接口。它们适合哈希表、无锁结构、序列化与分配器等底层代码；普通业务逻辑里若只是判断一个常量是不是 2 的幂，也值得用 `has_single_bit` 代替手写比较。
