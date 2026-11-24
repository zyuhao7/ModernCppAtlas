---
id: cpp20.integer-comparison
title: 安全整数比较（std::cmp_less 等）
standard: C++20
kind: library
category: [类型安全, 比较]
difficulty: 2
status: final
tier: A
compilers: {gcc: "10", clang: "13", msvc: "19.27"}
tags: [cmp_less, cmp_greater, cmp_equal, 符号安全, 整数比较]
alternatives: [static_cast 统一类型, 手写符号判断, std::in_range, GSL narrow]
proposal: P0586R2
last_verified: "2026-11-24"
claims:
  - id: c1
    statement: "用 cmp_less 比较 size_t 下标与 int 上限，避免隐式符号转换"
    evidence: [ev-stdexec-intcmp-loop-bound-001]
  - id: c2
    statement: "用 cmp_greater / cmp_less 检查时长计数是否超出目标取值范围"
    evidence: [ev-glaze-intcmp-chrono-range-002]
  - id: c3
    statement: "用 cmp_less_equal 断言有符号计数与无符号容量上限的关系"
    evidence: [ev-stdexec-intcmp-uring-assert-003]
review:
  author: "Codex"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

在 C++ 里直接比较一个 `int` 和一个 `unsigned`，编译器先做“通常算术转换”：把有符号值转成无符号。于是 `-1 < 1u` 结果是 `false`，因为 `-1` 变成了 `0xFFFFFFFF`。这类缺陷隐蔽，编译器通常只给一条 `-Wsign-compare` 警告，而 `static_cast` 统一类型又依赖程序员判断哪边安全。

C++20 的 `<utility>` 提供一组“数学意义上的整数比较”：`std::cmp_equal`、`cmp_not_equal`、`cmp_less`、`cmp_greater`、`cmp_less_equal`、`cmp_greater_equal`。它们按真实的数学值比较，不受符号转换影响；只要有符号值为负、而另一侧是无符号，就直接给出正确结论。六个函数都是 `constexpr` 与 `noexcept`。

## 基础用法

这些函数接收两个整数模板参数，类型可以不同；返回值是 `bool`。比较发生在“数值”层面，而不是转换后的位模式。

<!-- compile -->

```cpp
#include <iostream>
#include <utility>

int main() {
  const int a = -1;
  const unsigned b = 1;
  std::cout << std::cmp_less(a, b) << ' ' << std::cmp_greater(b, a) << '\n';
}
```

完整示例见 [`examples/cpp20/integer_comparison.cpp`](../../examples/cpp20/integer_comparison.cpp)，覆盖若干场景，输出为：

```text
[1] -1 as unsigned = 4294967295, cmp_less(-1, 1u) = 1
[2] cmp_greater(1u, -1) = 1
[3] cmp_equal(1u, 1) = 1, cmp_not_equal(1u, -1) = 1
[4] size_t 3 < int 5 = 1
[5] cmp_greater(int64_max, uint32_max) = 1
```

场景 1 先展示直接转换会把 -1 变成 4294967295，再用 `cmp_less(-1, 1u)` 得到数学上正确的 `true`。场景 2、3 分别演示大于与不等比较。场景 4 是常见情形：`size_t` 下标与 `int` 上限比较。场景 5 比较两个宽度不同的类型，`cmp_greater` 仍按数值判断。

## 进阶用法

**用在循环边界。** `for (std::size_t i = 0; std::cmp_less(i, nthreads); ++i)` 是典型写法：下标是 `size_t`、上限是 `int` 时，直接比较会有符号转换告警，`cmp_less` 按数值判断且语义清晰。

**范围检查与整数溢出防护。** 把目标类型的 `min()`/`max()` 与待转换值做 `cmp_less` / `cmp_greater`，可以在真正转换前判断是否溢出。协议解析、序列化读回、`chrono` 时长换算等场景常用这种模式，比先转换再比较更安全。

**约束到整数类型。** 六个函数只参与整数类型（标准整数与扩展整数）的重载决议。传给浮点或自定义类型不会按预期工作，泛型代码里对参数加 `std::integral` 约束可避免误用。

**不是 `std::in_range` 的替代。** `std::in_range<T>(x)` 只回答“能否无损转换到 `T`”；本页函数做的是任意两值的比较。需要检查转换安全性时优先用 `in_range`，两者用途不同。

## 真实项目证据

| 项目               | 固定源码                                                                                                                                                                                 | 用法                                               | 为什么典型                                     |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------- | ---------------------------------------------- |
| NVIDIA/stdexec     | [`examples/benchmark/common.hpp#L139-L140`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/examples/benchmark/common.hpp#L139-L140)                     | 用 `cmp_less` 比较 `size_t` 下标与 `int` 上限      | `size_t` 与 `int` 混比是符号转换告警的典型来源 |
| stephenberry/glaze | [`include/glaze/core/chrono.hpp#L519-L521`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/core/chrono.hpp#L519-L521)                 | 用 `cmp_greater` / `cmp_less` 判断时长是否溢出     | 计数与目标 `Duration::rep` 宽度、符号可能不同  |
| NVIDIA/stdexec     | [`include/exec/linux/io_uring_context.hpp#L508-L509`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/exec/linux/io_uring_context.hpp#L508-L509) | 用 `cmp_less_equal` 断言计数与无符号容量上限的关系 | 有符号计数对无符号容量的边界断言需要数学语义   |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **只在整数类型间比较。** 函数对浮点、枚举、自定义类型不适用；混入浮点时用这些函数会得到意外结果或编译错误。
- **不替代 `std::in_range`。** 本页函数比较两个具体值；检查“某值能否无损转换到目标整数类型”应使用 `std::in_range<T>(x)`。
- **不是所有比较都需要它。** 当两侧类型相同、或已经显式统一为同一类型时，普通 `==`、`<` 足够；`cmp_*` 只在“类型不同且可能涉及符号/宽度差异”时才有价值。
- **返回值是数学结果，不是转换结果。** `cmp_equal(-1, 0xFFFFFFFFu)` 为 `false`，即便两者的位模式在 32 位上相同；这正是与直接比较的关键区别。
- **旧工具链缺失。** GCC 10、Clang 13、MSVC 19.27 起才提供；更早编译器需要用显式符号判断或 GSL 的 `narrow` 等替代。
- **不要用它掩盖设计问题。** 频繁出现有符号/无符号混比，往往说明接口类型选择不一致；`cmp_*` 是安全网，不是鼓励随意混用类型的理由。

## 库联动

- **`std::in_range`**：同一批整数工具的另一半，回答“能否无损转换”，常与本节函数配合做范围校验。
- **`std::numeric_limits`**：取 `min()`/`max()` 作为范围检查的边界。
- **[std::span](cpp20.span.md) 与容器 `size()`**：`size()` 返回无符号，与有符号索引或上限比较时是本节函数的典型用武之地。
- **GSL `narrow` / `gsl::narrow_cast`**：在缺少 C++20 工具时的替代方案。
- **`<bit>` 的 `std::bit_cast`**：位重解释与数学比较互补，二者常一起出现在序列化代码里。

## 横向对比

| 方案                   | 受符号影响 | 可读性 | `constexpr` | 主要限制                   |
| ---------------------- | ---------- | ------ | ----------- | -------------------------- |
| `std::cmp_less` 等     | 否         | 高     | 是          | C++20 起；仅整数类型       |
| 直接 `a < b`           | 是         | 高     | 是          | 隐式符号转换导致错误结论   |
| `static_cast` 统一类型 | 取决于判断 | 中     | 是          | 需要人工确认哪侧转换安全   |
| `std::in_range`        | 否         | 高     | 是          | 只判断可转换性，不直接比较 |
| 手写符号分支           | 否         | 低     | 是          | 冗长，易漏边界             |

安全整数比较由 P0586R2 进入 C++20。它把“跨符号、跨宽度的整数比较”从容易出错的隐式转换里解放出来，适合解析器、序列化、计时与系统编程等频繁混用有符号/无符号类型的场景。若只是检查转换是否安全，优先考虑同批引入的 `std::in_range`。
