---
id: cpp20.endian
title: std::endian
standard: C++20
kind: library
category: [类型转换, 性能]
difficulty: 2
status: final
tier: A
compilers: {gcc: "8", clang: "7", msvc: "19.22"}
tags: [endian, 字节序, 序列化, constexpr, 可移植性]
alternatives: [__BYTE_ORDER__ 宏, boost::endian, 运行期探测, 手写联合体判断]
proposal: P0463R1
last_verified: "2026-11-13"
claims:
  - id: c1
    statement: "哈希算法用 std::endian::native 判断主机字节序，必要时翻转读入的 64 位值"
    evidence: [ev-mold-endian-siphash-read64-001]
  - id: c2
    statement: "十六进制打印用 if constexpr (std::endian::native == std::endian::little) 编译期选择字节遍历方向"
    evidence: [ev-nghttp2-endian-format-hex-002]
  - id: c3
    statement: "CBOR 编码用 consteval 函数按 std::endian::native 在编译期选择小端/大端标签"
    evidence: [ev-glaze-endian-cbor-native-tag-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

判断主机字节序过去靠非标准手段：`__BYTE_ORDER__`、`__LITTLE_ENDIAN` 这类编译器宏，或者运行期用 `union` / `reinterpret_cast` 探一位。宏依赖具体编译器，写成 `#ifdef` 到处散落；运行期探测会把一个编译期就该确定的事实推迟到运行时，既拖慢热路径也让常量表达式不可用。`boost::endian` 提供了完整方案，但要引入依赖。

C++20 的 `std::endian` 是一个 `enum class`，取值 `little`、`big`、`native`。`native` 描述主机字节序，是编译期常量，可以出现在 `if constexpr`、`static_assert` 和模板参数里；标准还要求 `native` 必须等于 `little` 或 `big` 之一，因此不用担心混合端序。它只描述事实，不做任何字节交换。

## 基础用法

`std::endian::native` 是主机字节序，`std::endian::little` / `std::endian::big` 是两个确定值。三者都是 `constexpr`，直接比较即可。

<!-- compile -->

```cpp
#include <bit>
#include <iostream>

int main() {
  std::cout << (std::endian::little != std::endian::big) << '\n';
}
```

输出：

```text
1
```

完整版本见 [`examples/cpp20/endian.cpp`](../../examples/cpp20/endian.cpp)，六个场景的输出为：

```text
[1] native is little or big = true
[2] little != big = true
[3] big-endian = 1 2 3 4
[4] little-endian = 4 3 2 1
[5] sizeof(uint8_t) = 1
[6] decoded = 0x1020304
```

场景 1 用 `static_assert` 确认 `native` 一定是 `little` 或 `big`。场景 2 展示两个枚举值互不相等。场景 3、4 把同一个 `0x01020304` 分别规范化成大端和小端字节序列，输出与主机端序无关。场景 5 说明单字节类型没有字节序问题。场景 6 从大端字节流反解回主机值。

## 进阶用法

**编译期分支。** 因为 `native` 是 `constexpr`，`if constexpr (std::endian::native == std::endian::little)` 会让另一个分支被完全丢弃，不会为无法到达的端序生成任何代码。这比运行期 `if` 更贴近意图，也避免了两端都编译。

**进入常量表达式。** `native` 可用在 `static_assert`、NTTP 和 `consteval` 函数里。glaze 的 CBOR 标签函数就是 `consteval`，在编译期根据 `native` 选择每个整型应该用的小端还是大端语义标签。

**它不交换字节。** `std::endian` 只回答“主机是哪种序”。真正改字节要另配 `std::byteswap`（C++23）、`memcpy` 加 `std::reverse`，或仓库自己的 `bswap`。mold 的 siphash 就是先判断 `native == big`，再调用自己的 `bswap` 翻转。

**单字节与网络序。** `uint8_t`、`char` 这类单字节类型没有字节序，判断其端序没有意义。跨网络或跨文件交换多字节数据时，约定一个固定序（通常是网络序即大端），在本机是另一种序时才翻转。

**不是一个“转换”设施。** 把它和 `std::bit_cast` 并用时分工明确：`endian` 回答端序问题，`bit_cast` 在类型之间搬运位模式，两者都不替你做序列化。

## 真实项目证据

| 项目    | 位置                                                                                                                                                                     | 用法                                                                     | 为什么典型                                                 |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------ | ---------------------------------------------------------- |
| mold    | [`lib/siphash.h#L86-L92`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/siphash.h#L86-L92)                                            | 用 `std::endian::native` 判断主机字节序，必要时翻转读入的 64 位值        | 哈希算法要求固定字节序，宏或运行期探测都不适合编译期判断   |
| nghttp2 | [`src/util.h#L456-L469`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/util.h#L456-L469)                                          | `if constexpr (std::endian::native == std::endian::little)` 选择遍历方向 | 输出顺序取决于端序，编译期分支让另一种端序的代码根本不生成 |
| glaze   | [`include/glaze/cbor/header.hpp#L256-L270`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/cbor/header.hpp#L256-L270) | 用 `consteval` 函数按 `std::endian::native` 在编译期选择 CBOR 标签       | 标签在常量表达式里算出，要求端序判断本身是 `constexpr`     |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三条都在生产路径上，覆盖哈希、十六进制输出与序列化三种端序用法。

## 陷阱与边界

**它不负责字节交换。** 判断出主机是大端后，要翻转成小端仍需你自己动手；`std::endian` 不会悄悄替你转换任何数据。

**不要假设主机是小端。** 大量代码在 x86 上开发，隐含假定小端。写可移植代码时用 `if constexpr` 覆盖两种端序，或统一规范化到固定序，别把 `native` 当成 `little`。

**浮点与结构体的布局更复杂。** `std::endian` 只描述整型的字节序；浮点表示、结构体填充、位域布局是另外的表示问题，跨平台交换时不能仅靠 `endian` 判断。

**混合端序在实际目标上几乎不存在。** 标准要求 `native` 等于 `little` 或 `big` 之一，这两者之外的“混合”端序不被支持，比较时无需为它写分支。

**旧工具链没有它。** GCC 8、Clang 7（libc++）、MSVC 19.22 起才提供。更早的编译器要退回 `__BYTE_ORDER__`、`boost::endian` 或运行期探测。

## 库联动

- **`std::bit_cast`**：在类型间搬运位模式，与 `endian` 分工——一个管端序判断，一个管位的复制。
- **`boost::endian`**：提供字节序判断与整型读写的一体化方案，是旧标准下的替代来源。
- **`std::byteswap`**（C++23）：直接翻转整型的字节序，正好接在 `endian` 判断之后。
- **序列化库**：CBOR、BSON、MessagePack 一类库用 `endian` 决定线格式的字节序（glaze 即此用法）。

## 横向对比

| 方案                  | `constexpr` | 角色         | 依赖宏 | 主要限制                       |
| --------------------- | ----------- | ------------ | ------ | ------------------------------ |
| `std::endian`         | 是          | 只判断端序   | 否     | C++20 起；不做字节交换         |
| `__BYTE_ORDER__` 等宏 | 是          | 只判断端序   | 是     | 非标准，依赖具体编译器         |
| 运行期 `union` 探测   | 否          | 判断并给出值 | 否     | 不能在常量表达式用，进入热路径 |
| `boost::endian`       | 是          | 判断并读写   | 否     | 引入依赖，接口面积更大         |

`std::endian` 由 P0463R1 进入 C++20，把“主机是哪种字节序”从平台宏和运行期探测收敛成一个标准、可 `constexpr` 的枚举。它刻意只回答问题、不做转换，把字节交换留给 `byteswap` 或调用方，从而在编译期就能剔除用不到的分支。
