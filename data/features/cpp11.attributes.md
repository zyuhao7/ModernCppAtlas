---
id: cpp11.attributes
title: 标准属性（attributes）
standard: C++11
kind: language
category: [语言机制, 诊断]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.8", clang: "3.3", msvc: "19.0"}
tags: [属性, noreturn, 编译器提示, 可移植性]
alternatives: [编译器扩展属性, 预处理器宏, 后续标准属性, 不使用属性]
proposal: N2761
last_verified: "2026-11-07"
claims:
  - id: c1
    statement: "用 [[noreturn]] 标注总是抛异常的日志错误辅助函数"
    evidence: [ev-spdlog-noreturn-throw-001]
  - id: c2
    statement: "在异常委托接口上批量标注 [[noreturn]]"
    evidence: [ev-abseil-noreturn-throw-delegate-002]
  - id: c3
    statement: "用 [[noreturn]] 标注直接终止进程的子命令入口"
    evidence: [ev-mold-noreturn-subcommand-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

编译器需要额外信息才能做更好的诊断和优化：某个函数永不返回、某个返回值不应被忽略、某段代码可能无人使用。C++11 之前这些信息靠厂商扩展表达，GNU 用 `__attribute__((noreturn))`，MSVC 用 `__declspec(noreturn)`，语法和语义各不相同，跨平台代码只能靠宏拼接。

C++11 引入统一的属性语法 `[[...]]`，把这类提示变成语言的一部分。未知属性可以被忽略，厂商扩展也能借这套语法逐步收敛。C++11 标准化了 `[[noreturn]]` 与 `[[carries_dependency]]`，后续标准继续扩充列表。标准提案为 N2761。

## 基础用法

属性写在声明前，`[[noreturn]]` 告诉编译器该函数不会正常返回；它可以抛出异常或直接终止进程，但不能执行到函数末尾。

<!-- compile -->

```cpp
#include <iostream>
#include <stdexcept>

[[noreturn]] void fail(const char* message) {
  throw std::runtime_error(message);
}

int main() {
  try {
    fail("boom");
  } catch (const std::exception& error) {
    std::cout << error.what() << '\n';
  }
}
```

输出 `boom`。`fail` 只有抛出这一条路径，标注后编译器不会为“函数返回”生成后续分支，也不会在调用点报“控制流到达末尾”的警告。完整版本见 [`examples/cpp11/attributes.cpp`](../../examples/cpp11/attributes.cpp)。

## 进阶用法

**属性作用于声明。** `[[noreturn]]` 放在函数声明或定义之前，也可以修饰函数指针、lambda 的调用运算符等；同一实体在多处声明时，属性应当一致，避免一个翻译单元看到契约而另一个看不到。

**未知属性可被忽略。** 标准允许实现忽略不认识的属性；带命名空间的属性如 `[[gnu::always_inline]]` 是厂商扩展，换编译器可能完全不起作用。可移植代码只依赖标准属性。

**`[[noreturn]]` 是契约不是优化提示。** 如果函数实际返回，程序行为未定义；编译器可能据此删掉调用点之后的代码，误用比不加属性更危险。

**`[[carries_dependency]]` 与内存序。** 它用于 `std::memory_order_consume` 的依赖传递，但 `consume` 在实践中被多数实现提升为 `acquire`，后续标准也已弃用，现代代码很少需要这个属性。

**后续标准扩充属性表。** C++14 加入 `[[deprecated]]`，C++17 加入 `[[nodiscard]]`、`[[fallthrough]]`、`[[maybe_unused]]`，C++20 加入 `[[likely]]`、`[[unlikely]]`、`[[no_unique_address]]`。同一套语法贯穿各标准版本。

**属性可以带参数。** `[[deprecated("use parse_v2 instead")]]` 的字符串会出现在诊断信息里；自定义命名空间属性也可以带参数，由对应工具解释。

## 真实项目证据

| 项目   | 位置                                                                                                                                                          | 用法                           | 为什么典型                                   |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------ | -------------------------------------------- |
| spdlog | [`include/spdlog/common.h#L308-L313`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/common.h#L308-L313)       | 抛异常的错误辅助函数标注不返回 | 调用点之后不可达，减少警告和冗余分支         |
| abseil | [`absl/base/throw_delegate.h#L39-L47`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/base/throw_delegate.h#L39-L47) | 一组抛出标准异常的委托函数     | 属性是接口契约的一部分，支持无异常构建       |
| mold   | [`src/mold.h#L1588-L1590`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/mold.h#L1588-L1590)                               | 终止进程的子命令入口           | 属性放在模板声明与返回类型之间，展示标准位置 |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖日志错误、异常委托与命令行入口三种不返回语义。

## 陷阱与边界

**说谎的 `[[noreturn]]` 是未定义行为。** 函数一旦返回，程序不再有任何保证；重构时删掉最后的 `throw` 或 `std::exit` 却忘了改属性，编译器仍会按不返回生成代码。

**它不是 `noexcept`。** `[[noreturn]]` 只描述“不返回”，函数仍可以抛异常；需要同时表达不抛异常时，两个说明符要分别写。

**属性不改变 ABI。** 它不参与重载决议，也不改变函数签名；头文件里加了属性，调用方不重新编译也能链接，但优化和诊断效果不会生效。

**未知属性可能静默失效。** 换编译器或标准库后，厂商属性可能被忽略；不要用属性实现正确性逻辑，只把它当作提示或契约。

**放置位置要保守。** 标准属性放在声明开头最稳妥；把属性塞进声明符中间或返回类型之后，不同编译器的接受程度不同。

**MSVC 支持较晚。** 标准属性要到 VS2015（19.0）才完整可用；需要兼容更老的 MSVC 时只能继续用 `__declspec` 宏。

**不要用属性替代 `static_assert`。** 属性是编译期提示，不能做条件校验；需要阻止错误实例化时仍然使用 `static_assert` 或 SFINAE。

## 库联动

- **`<cstdlib>`**：`std::abort`、`std::exit` 是 `[[noreturn]]` 函数的典型实现，包装它们时属性要与实际行为一致。
- **`<stdexcept>`**：抛异常的函数同样满足不返回契约，但不要误以为属性会阻止异常传播。
- **`<cassert>`**：断言失败路径常用 `[[noreturn]]` 辅助函数收尾，避免调用点出现不可达代码警告。
- **编译器诊断**：`-Wattributes`、`/W4` 会对属性误用给出警告，CI 中应把相关警告当作错误处理。
- **后续标准属性**：`[[nodiscard]]`、`[[deprecated]]` 等让同一套语法覆盖更多诊断场景。
- **spdlog / abseil / mold**：分别展示日志错误、异常委托与子命令入口三类 `[[noreturn]]` 用法。

## 横向对比

| 方案                | 可移植性  | 表达力            | 主要限制                           |
| ------------------- | --------- | ----------------- | ---------------------------------- |
| 标准属性 `[[...]]`  | 标准      | 标准属性集合      | 旧编译器支持晚，未知属性可能被忽略 |
| GNU `__attribute__` | GCC/Clang | 非常丰富          | 语法非标准，其他编译器不识别       |
| MSVC `__declspec`   | MSVC      | 覆盖 Windows 场景 | 与 GNU 语法不兼容                  |
| 预处理器宏          | 取决于宏  | 可封装差异        | 可读性差，容易在模板里展开错误     |

跨标准演进：C++11 通过 N2761 引入统一属性语法与 `[[noreturn]]`；C++14、C++17、C++20 逐步加入 `[[deprecated]]`、`[[nodiscard]]`、`[[likely]]` 等属性；C++23 继续补充与生命周期和假设相关的属性。语法保持不变，属性表持续扩充。
