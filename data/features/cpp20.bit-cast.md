---
id: cpp20.bit-cast
title: std::bit_cast
standard: C++20
kind: library
category: [类型转换]
difficulty: 3
status: final
tier: A
compilers: {gcc: "11", clang: "14", msvc: "19.27"}
tags: [bit_cast, 位表示, memcpy, 常量求值, 类型双关]
alternatives: [memcpy, reinterpret_cast, union, std::start_lifetime_as]
proposal: P0476R2
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "兼容层在常量求值时调用 std::bit_cast，运行期回退到 memcpy"
    evidence: [ev-fmt-bit-cast-fallback-001]
  - id: c2
    statement: "特性可用时把 absl::bit_cast 别名到 std::bit_cast"
    evidence: [ev-abseil-bit-cast-alias-002]
  - id: c3
    statement: "用 bit_cast 在有符号与无符号整数之间按位重解释"
    evidence: [ev-abseil-bit-cast-endian-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

需要在两种类型之间按位重解释时，常见写法都有问题。`reinterpret_cast` 会破坏严格别名规则；`memcpy` 能避免别名问题，但不能用于常量求值；用 `union` 做类型双关在 C++ 中是未定义行为。

`std::bit_cast<To>(from)` 复制对象的位表示，要求 `To` 与 `From` 大小相同且都是可平凡复制类型。它在 C++20 中是 `constexpr`，因此可以在编译期做位级转换，也可以安全地在运行期使用。

## 基础用法

<!-- compile -->

```cpp
#include <bit>
#include <cstdint>
#include <iostream>

int main() {
  std::uint32_t bits = 0x3f800000;
  float value = std::bit_cast<float>(bits);
  std::cout << value << '\n';
}
```

`0x3f800000` 是 IEEE 754 单精度 `1.0f` 的位表示；`bit_cast` 把它复制为 `float`，不经过数值转换。换一种类型或改变位模式，结果也随之改变。

## 进阶用法

`bit_cast` 是 `constexpr`，可以在编译期检查位模式，也可以把浮点拆成符号、指数、尾数三段。

```cpp
constexpr std::uint32_t bits = std::bit_cast<std::uint32_t>(1.0f);
static_assert(bits == 0x3f800000u);

std::uint32_t raw = 0x40490fdb;
float value = std::bit_cast<float>(raw);
std::uint32_t sign = raw >> 31;
std::uint32_t exponent = (raw >> 23) & 0xff;
std::uint32_t mantissa = raw & 0x7fffff;
```

`static_assert` 在编译期验证 IEEE 754 布局；运行期代码用同样的 `bit_cast` 读取原始位，再用移位和掩码拆字段。这里假设 `float` 是 32 位 IEEE 754，跨平台代码应先检查 `std::numeric_limits<float>::is_iec559` 与 `sizeof(float)`。

多字节类型的字节序由平台决定。需要跨平台交换二进制数据时，先用 `std::endian` 判断端序，再决定是否交换字节；`bit_cast` 本身不负责序列化。

完整示例见 [`examples/cpp20/bit_cast.cpp`](../../examples/cpp20/bit_cast.cpp)。

## 真实项目证据

| 项目       | 位置                                                                                                                                                                | 用法                                                  | 为什么典型                                                  |
| ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------- | ----------------------------------------------------------- |
| fmt        | [`include/fmt/format.h#L266-L276`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/format.h#L266-L276)                      | 常量求值时调用 `std::bit_cast`，运行期回退到 `memcpy` | 兼容层要支持 C++17，同时让 C++20 的常量求值路径使用标准实现 |
| abseil-cpp | [`absl/base/casts.h#L168-L175`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/base/casts.h#L168-L175)                     | 特性可用时把 `absl::bit_cast` 别名到 `std::bit_cast`  | 保留旧接口名，同时让 C++20 用户直接使用标准实现             |
| abseil-cpp | [`absl/base/internal/endian.h#L139-L147`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/base/internal/endian.h#L139-L147) | 用 `bit_cast` 在有符号与无符号整数间搬运位模式        | 端序转换不能改变位模式，数值转换会破坏原始字节              |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。前两条展示兼容层，第三条展示运行期位级转换的生产用法。

## 陷阱与边界

**大小必须相同。** `sizeof(To) == sizeof(From)` 是硬性要求；`int` 与 `long` 在部分平台上大小不同，不能直接互转。需要变宽或变窄时，先明确数值转换语义，而不是按位扩展。

**只接受可平凡复制类型。** 带虚函数、自定义拷贝构造或非平凡析构的类型不能用 `bit_cast`；它不会调用构造函数，也不会建立新的对象生命周期。

**位模式必须对目标类型有效。** `bit_cast` 只复制表示；如果目标类型的某些位模式是陷阱表示或无效值，后续使用仍可能未定义。不要用任意整数构造浮点或指针值。

**端序影响多字节结果。** `bit_cast` 按对象表示复制，多字节类型的字节序由平台决定；网络协议和文件格式要显式处理端序，不能把 `bit_cast` 当作可移植序列化。

**填充字节也会被复制。** 结构体中的填充位内容不确定；按位比较或哈希包含填充的结构体时，结果不稳定。先规范化或逐字段处理。

**`constexpr` 能力依赖实现。** C++20 起 `bit_cast` 是 `constexpr`，但旧标准库或编译器可能只提供运行期版本；需要常量求值时用 `__cpp_lib_bit_cast` 和编译器实际行为确认。

**不要再用 union 或 `reinterpret_cast` 替代。** union 类型双关在 C++ 中不是合法替代；`reinterpret_cast` 还会触发严格别名问题。需要按位复制时，`memcpy` 是运行期回退，`bit_cast` 是更清晰的表达。

## 库联动

- **fmt**：常量求值时使用 `std::bit_cast`，运行期回退到 `memcpy`，让 C++17 与 C++20 共用一套位转换工具。
- **Abseil**：`casts.h` 把兼容名别名到标准实现，`endian.h` 用 `bit_cast` 在有符号与无符号整数间搬运位模式。
- **与 `std::endian` 的分工**：`bit_cast` 只复制对象表示，不判断或转换端序；跨平台读写二进制协议时，端序判断要单独完成。

## 横向对比

| 方案                     | 严格别名安全 | 常量求值 | 大小检查 | 主要代价                                |
| ------------------------ | ------------ | -------- | -------- | --------------------------------------- |
| `std::bit_cast`          | 是           | 是       | 编译期   | 要求可平凡复制且大小相同                |
| `memcpy`                 | 是           | 否       | 手动     | 运行期调用；需要自己保证大小与生命周期  |
| `reinterpret_cast`       | 否           | 否       | 无       | 可能触发未定义行为，不能用于常量求值    |
| `union` 类型双关         | 否           | 部分     | 无       | C++ 中读取非活跃成员是未定义行为        |
| `std::start_lifetime_as` | 是           | 是       | 手动     | C++23；用于在已有存储上开始对象生命周期 |

跨标准演进：C++20 引入 `std::bit_cast`；C++23 增加 `std::start_lifetime_as`，解决“在已有字节缓冲区上创建对象”的另一类问题。两者不能互相替代：`bit_cast` 复制值表示，`start_lifetime_as` 管理对象生命周期。
