---
id: cpp11.alignas-alignof
title: 对齐控制（alignas 与 alignof）
standard: C++11
kind: language
category: [内存管理, 语言机制]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.8", clang: "3.0", msvc: "19.0"}
tags: [alignas, alignof, 对齐, 缓存行, 伪共享]
alternatives: [编译器扩展属性, 手动填充, std::aligned_storage, 对齐分配器]
proposal: N2341
last_verified: "2026-11-07"
claims:
  - id: c1
    statement: "用 alignas(2) 对齐两位数查表数据"
    evidence: [ev-fmt-alignas-digit-table-001]
  - id: c2
    statement: "用 alignas(32) 对齐哈希表条目并按 alignof 分配内存"
    evidence: [ev-mold-alignas-hash-entry-002]
  - id: c3
    statement: "用 alignas(64) 对齐多生产者单消费者原子队列"
    evidence: [ev-stdexec-alignas-mpmc-queue-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

SIMD 指令、原子操作和缓存行都关心地址对齐：未对齐访问可能更慢，跨缓存行的原子变量会产生伪共享，某些指令甚至要求对齐否则触发异常。C++11 之前只能依赖 `__attribute__((aligned))`、`__declspec(align)` 等编译器扩展，代码不可移植，也无法在类型系统里表达“这个对象至少要对齐到多少”。

`alignof` 查询类型的对齐要求，`alignas` 在声明处提出更严格的对齐要求。两者是同一个提案引入的语言级工具，让对齐成为类型和对象声明的一部分，而不是散落的平台宏。标准提案为 N2341。

## 基础用法

`alignof(T)` 返回类型 `T` 的对齐要求，`alignas(N)` 把变量或类型对齐到至少 `N` 字节；`N` 必须是 2 的幂，且不能弱于类型的自然对齐。

<!-- compile -->

```cpp
#include <cstdint>
#include <iostream>

struct alignas(16) Vec4 {
  float values[4];
};

int main() {
  std::cout << alignof(Vec4) << ' ' << sizeof(Vec4) << '\n';
  alignas(32) int buffer[8] = {};
  std::cout << (reinterpret_cast<std::uintptr_t>(buffer) % 32 == 0) << '\n';
}
```

输出 `16 16` 与 `1`。完整版本见 [`examples/cpp11/alignas_alignof.cpp`](../../examples/cpp11/alignas_alignof.cpp)，六个场景的输出为：

```text
[1] alignof(Vec4) = 16, sizeof(Vec4) = 16
[2] alignof(Padded) = 16, sizeof(Padded) = 16
[3] alignof(CacheLine) = 64, stride = 64, both aligned = 1
[4] buffer % 64 = 0, buffer[3] = 4
[5] align_report<CacheLine>::value = 64, static_assert passed = 1
[6] counter = 7, counter % 32 = 0
```

场景 1 把 `Vec4` 的对齐提升到 16 字节，大小仍是四个 `float` 的 16 字节，两者恰好相等。场景 2 是关键对照：`Padded` 只有一个 `char`，但 `alignas(16)` 让 `sizeof` 必须是对齐的整数倍，于是从 1 字节膨胀到 16 字节。场景 3 的 `CacheLine` 按 64 字节对齐，数组跨度也是 64，两个元素的地址都落在缓存行起点，热点之间不会共享缓存行。场景 4 证明多条 `alignas` 取最严格者：`alignas(8)` 被 `alignas(64)` 覆盖，整个数组按 64 字节对齐，元素内容不受影响。场景 5 说明 `alignof` 是编译期常量，可以进入模板参数和 `static_assert`。场景 6 把同样的要求用在局部变量上。

## 进阶用法

**对齐类型而不只是变量。** `struct alignas(64) Counter` 让该类型的每个对象都满足 64 字节对齐，包括数组元素和动态分配的对象；把热点数据按缓存行隔离是避免伪共享的常用手法。

**多条 `alignas` 取最严格者。** 声明上可以写多个 `alignas`，最终要求是所有值中最大的那个；自然对齐仍然生效，`alignas` 只能加强不能削弱。

**对齐动态分配。** C++17 起 `new` 表达式会为过对齐类型选择带 `std::align_val_t` 的 `operator new`；在此之前，普通 `new` 只保证默认对齐，过对齐类型需要 `posix_memalign`、`_aligned_malloc` 或自定义分配器。

**用 `alignof` 保持分配契约。** 手工分配缓冲区时把 `alignof(T)` 传给对齐分配函数，保证后续按 `T*` 使用时满足类型要求，mold 的证据正是这种写法。

**查询缓存行参数。** C++17 提供 `std::hardware_destructive_interference_size` 和 `hardware_constructive_interference_size`，用它们代替硬编码的 64 字节，能让代码适应目标平台。

**`std::align` 在缓冲区里找对齐位置。** 需要在已有字节缓冲区中切出对齐子区间时，`std::align` 返回调整后的指针并更新剩余空间，比手动取模更安全。

## 真实项目证据

| 项目    | 位置                                                                                                                                                                                                             | 用法                                      | 为什么典型                                     |
| ------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------- | ---------------------------------------------- |
| fmt     | [`include/fmt/format.h#L1015-L1027`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/format.h#L1015-L1027)                                                               | 两位数查表按 2 字节对齐                   | 格式化热路径，跨边界未对齐访问更慢             |
| mold    | [`lib/common.h#L576-L590`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/common.h#L576-L590)                                                                                  | 哈希条目按 32 字节对齐并按 `alignof` 分配 | 并发哈希表减少缓存行伪共享，分配与类型契约一致 |
| stdexec | [`include/stdexec/__detail/__atomic_intrusive_queue.hpp#L31-L45`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__atomic_intrusive_queue.hpp#L31-L45) | 原子队列按 64 字节对齐                    | 头指针是跨线程 CAS 热点，隔离到独立缓存行      |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖查表布局、并发容器和原子队列三种对齐动机。

## 陷阱与边界

**过对齐的动态分配需要配套支持。** C++17 之前普通 `new` 不保证超过默认对齐的地址，把过对齐类型放进 `std::vector` 或裸 `new` 可能得到错误对齐；确认工具链和分配器是否支持 `align_val_t`。

**`alignas` 不能降低自然对齐。** 对 `double` 写 `alignas(1)` 是无效的；`alignas` 只能提出更强要求，想压缩布局要靠改变成员类型或打包扩展。

**对齐会放大对象尺寸。** 更高的对齐可能引入尾部填充，数组和容器的内存占用随之增加；按缓存行隔离热点时要把这份开销算进去。

**伪共享不是语言语义。** 对齐只是让对象落在不同缓存行，是否真的避免伪共享还取决于缓存行宽度和访问模式；用性能计数器验证，而不是只看 `alignas` 数字。

**跨翻译单元必须一致。** 类型的对齐是其 ABI 的一部分，头文件里改 `alignas` 而某个翻译单元没重新编译，会得到布局不一致的对象；修改后要整体重新构建。

**`alignof` 对不完整类型不成立。** 只有前置声明的类型无法查询对齐；模板里对不完整类型使用 `alignof` 也会失败，需要推迟到类型完整处。

**SIMD 代码不要假设对齐总是必要。** 现代 x86 的未对齐加载通常可用，但跨缓存行仍可能变慢；ARM 等平台对某些指令更严格，跨平台向量代码应同时处理对齐与未对齐路径。

## 库联动

- **`<cstddef>`**：`std::max_align_t`、`std::align` 与 `std::byte` 所在头文件，手工缓冲区管理常用。
- **`<memory>`**：分配器与 `std::allocator_traits` 决定容器内元素的对齐；过对齐类型需要支持对齐的分配器。
- **`<new>`**：C++17 的带对齐 `operator new` 与 `std::align_val_t` 让动态分配遵守 `alignas`。
- **`<bit>`**：C++20 的 `std::bit_cast`、`std::has_single_bit` 常与对齐和布局检查一起使用。
- **SIMD 库**：Eigen、xsimd 等要求输入缓冲区满足特定对齐，调用方用 `alignas` 或对齐分配器准备数据。
- **fmt / mold / stdexec**：分别展示查表、并发哈希表和原子队列中的对齐动机。

## 横向对比

| 方案                   | 可移植性           | 表达能力       | 主要限制                               |
| ---------------------- | ------------------ | -------------- | -------------------------------------- |
| `alignas` / `alignof`  | 标准               | 类型与变量级   | 不能降低自然对齐，过对齐分配需配套支持 |
| 编译器扩展属性         | 各平台             | 类似，语法不同 | 不可移植，宏封装后仍难统一             |
| 手动填充               | 标准               | 只能调整大小   | 不保证起始地址，易算错                 |
| `std::aligned_storage` | 标准（C++23 弃用） | 提供对齐存储   | 已弃用，新代码应改用 `alignas`         |
| 对齐分配器             | 标准               | 动态对象级     | 需要额外依赖或自定义分配逻辑           |

跨标准演进：C++11 通过 N2341 引入 `alignas` / `alignof`；C++17 补齐过对齐的 `new` 并加入缓存行干扰常量；C++20 的 `std::assume_aligned` 把对齐信息传给优化器；C++23 弃用 `std::aligned_storage`，推荐直接用 `alignas` 定义存储类型。
