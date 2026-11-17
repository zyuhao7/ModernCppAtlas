---
id: cpp23.byteswap
title: std::byteswap
standard: C++23
kind: library
category: [类型转换, 性能]
difficulty: 2
status: final
tier: A
compilers: {gcc: "12", clang: "14", msvc: "19.31"}
tags: [byteswap, 字节序, 端序, 序列化, 位运算]
alternatives: [ntohl, htonl, __builtin_bswap32, 手动逐字节交换, std::reverse]
proposal: P1272R4
last_verified: "2026-11-15"
claims:
  - id: c1
    statement: "二进制反序列化用特性测试宏选择 std::byteswap，旧标准库回退到逐字节交换"
    evidence: [ev-nlohmann-byteswap-byte-swap-001]
  - id: c2
    statement: "写小端线格式前按主机端序判断，必要时对无符号版本调用 std::byteswap"
    evidence: [ev-glaze-byteswap-bson-le-002]
  - id: c3
    statement: "库在命名空间内用 using std::byteswap 暴露标准实现，旧标准库走本地回退"
    evidence: [ev-abseil-byteswap-using-decl-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

跨平台交换数据时，同一个整数在不同主机上的内存字节顺序可能相反。把值按网络字节序写出、从二进制文件读入、或者解析固定宽度的线格式，都需要把字节顺序翻转过来。过去这套逻辑散落在 `ntohl` / `htonl` 这类平台函数、编译器内建（如 `__builtin_bswap32`）和手写移位循环之间，语义不统一，还容易在窄整数上触发整型提升问题。

`std::byteswap` 为所有整型提供统一的字节翻转语义。它定义在 `<bit>` 中，参与重载决议时只接受整型，返回与实参同类型的值；对单字节类型原样返回。P1272R4 把它加入 C++23，让“交换字节顺序”成为标准库里一个可推导、可 `constexpr` 求值的操作。

## 基础用法

`std::byteswap(x)` 接受整型 `x`，按字节翻转其表示。翻转两次会得到原值。

<!-- compile -->

```cpp
#include <bit>
#include <cstdint>
#include <iostream>

int main() {
  std::uint16_t value = 0x1234;
  std::cout << std::hex << std::byteswap(value) << '\n';
}
```

输出：

```text
3412
```

完整版本见 [`examples/cpp23/byteswap.cpp`](../../examples/cpp23/byteswap.cpp)，六个场景的输出为：

```text
[1] 3412
[2] 04030201
[3] round-trip = true
[4] ab
[5] big-endian value = 04030201
[6] ff00
```

场景 1 翻转 16 位值，场景 2 翻转 32 位值，字节顺序整体颠倒。场景 3 验证两次翻转还原原值。场景 4 说明 `std::uint8_t` 这类单字节类型不受影响。场景 5 在 `constexpr` 上下文中使用它，并用 `static_assert` 校验结果。场景 6 展示前导零字节在翻转后跑到末尾。

## 进阶用法

**只在需要时交换。** 主机端序与线格式一致时，交换是多余开销。glaze 写 BSON 小端格式时用 `if constexpr` 判断 `std::endian::native == std::endian::big`，只在主机为大端时才调用 `std::byteswap`，这一分支在编译期就被消除。

**先转无符号再交换。** 对有符号整数，标准本身允许交换，但结果依赖补码表示，可移植性差。glaze 的做法是 `static_cast<std::make_unsigned_t<T>>(value)` 后再交换，再转回原类型，避免有符号值的表示与移位问题。

**用特性测试宏做兼容层。** 新函数需要旧标准库兼容。nlohmann/json 在 `byte_swap` 里用 `__cpp_lib_byteswap` 判断，可用时对整型调用 `std::byteswap`，不可用时继续走下方的逐字节循环；单字节情况提前返回。

**库内暴露统一名字。** abseil 在 `__cpp_lib_byteswap >= 202110L` 时直接 `using std::byteswap;`，否则在下方提供等价实现。调用方只看到 `absl::byteswap`，不关心背后是标准实现还是本地回退。

**可用于常量表达式。** `std::byteswap` 是 `constexpr`，适合在编译期生成查表、校验字节序常量或拼接固定宽度的头部字段。

## 真实项目证据

| 项目               | 位置                                                                                                                                                                                                        | 用法                                                                   | 为什么典型                                                 |
| ------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------- | ---------------------------------------------------------- |
| nlohmann/json      | [`include/nlohmann/detail/input/binary_reader.hpp#L2820-L2834`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/detail/input/binary_reader.hpp#L2820-L2834) | 特性测试宏判断后对整型调用 `std::byteswap`，否则回退逐字节交换         | 二进制反序列化路径，展现新标准函数的向后兼容写法           |
| stephenberry/glaze | [`include/glaze/bson/write.hpp#L67-L75`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/bson/write.hpp#L67-L75)                                          | 按主机端序 `if constexpr` 判断，必要时对无符号版本调用 `std::byteswap` | 小端线格式写入，端序分支在编译期消除，交换只对整数类型启用 |
| abseil/abseil-cpp  | [`absl/numeric/bits.h#L217-L226`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/numeric/bits.h#L217-L226)                                                         | 标准可用时 `using std::byteswap;`，不可用时在下方提供等价实现          | 跨标准兼容层，调用方看到统一的 `absl::byteswap`            |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。两处来自生产路径，覆盖二进制反序列化、线格式写入和库内兼容层三种场景。

## 陷阱与边界

**只接受整型。** 枚举、浮点数和 `std::byte` 都不满足约束。枚举可以先转其底层类型，浮点数应使用 `std::bit_cast` 到等宽整数后再交换。

**结果类型不变。** `std::byteswap` 返回与实参相同的类型，不会悄悄加宽。对 `std::uint8_t` 调用得到原值；若想以数值形式输出，记得先转成 `int`，否则 `operator<<` 会按字符处理。

**有符号值的表示问题。** 标准允许对任意整型交换，但有符号负数的字节表示依赖补码；需要可移植时先转无符号类型。

**不处理端序本身。** 它只翻转字节，不知道主机是小端还是大端。判断端序仍要用 `std::endian`，二者通常配合使用。

**填充位的类型不保证可用。** 实现可能拒绝含有填充位的整数类型；常规的 `std::uintN_t` 没有这个问题。

**旧工具链没有它。** GCC 12、Clang 14、MSVC 19.31 起提供。更早版本只能用编译器内建或逐字节循环。

## 库联动

- **`std::endian`**：判断主机端序，决定是否需要交换；与 `std::byteswap` 是固定搭配。
- **`std::bit_cast`**：在浮点与整数之间搬运位模式，再交给 `std::byteswap` 处理端序。
- **`std::make_unsigned_t`**：把有符号整数转成无符号版本再交换，规避表示差异。
- **`__cpp_lib_byteswap`**：特性测试宏，用于在新旧标准库之间选择实现。

## 横向对比

| 方案                   | 适用范围       | 可移植性 | `constexpr` | 主要限制                       |
| ---------------------- | -------------- | -------- | ----------- | ------------------------------ |
| `std::byteswap`        | 所有整型       | 高       | 是          | C++23 起；仅整型               |
| `ntohl` / `htonl`      | 32 位网络序    | 中       | 否          | 平台函数，宽度与主机端序绑定   |
| `__builtin_bswap32`    | 特定宽度的整数 | 低       | 是（内建）  | 编译器扩展，需要按宽度选择名字 |
| 逐字节循环 + `reverse` | 任意字节序列   | 高       | 是          | 手写代码，容易遗漏类型宽度     |

`std::byteswap` 由 P1272R4 进入 C++23，把字节翻转从平台函数和编译器内建收敛成一个类型安全的标准操作。它不判断端序、不改变宽度、也不接受非整型，边界清楚，正好适合序列化与线格式这类需要精确控制字节表示的场景。
