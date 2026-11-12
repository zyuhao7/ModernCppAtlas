---
id: cpp11.error-code
title: std::error_code
standard: C++11
kind: library
category: [错误处理, 诊断]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.4", clang: "3.0", msvc: "16.0"}
tags: [error_code, system_error, errc, 错误类别, 无异常]
alternatives: [异常, std::expected, 整型返回码, errno, tl::expected, absl::StatusOr]
proposal: N2241
last_verified: "2026-11-12"
claims:
  - id: c1
    statement: "文件系统探测用输出型 error_code 区分成功与其他失败"
    evidence: [ev-mold-error-code-filesystem-001]
  - id: c2
    statement: "目录创建失败时读取 error_code 并记录诊断后终止"
    evidence: [ev-drogon-error-code-create-dirs-002]
  - id: c3
    statement: "把 error_code 适配成需要抛出的 system_error"
    evidence: [ev-stdexec-error-code-system-error-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

不启用异常时，错误常靠整型返回码或 `errno` 传递。返回码把正常值和错误码塞进同一个整数，调用方必须翻文档才能知道哪些值算失败；`errno` 是线程局部的全局状态，调用前后都要小心保存和清零，而且不同库对同一个整数可能赋予完全不同的含义。

`std::error_code` 把错误值和一个错误类别绑定在一起。类别说明“这个整数属于哪个错误空间”，值说明“具体是哪个错误”；比较时两者都参与，因此 `7` 来自通用类别和 `7` 来自系统类别不会被混为一谈。错误作为返回值正常传递，不需要异常，也不需要全局状态。

配套的 `std::system_error` 把 `error_code` 包成异常；需要抛出的接口仍然可以携带错误码和类别信息，调用方既能捕获异常，也能直接检查返回值。

## 基础用法

`std::error_code` 默认构造表示成功，`operator bool` 为假；非零值表示失败。`std::errc` 是标准定义的通用错误枚举，通过 `std::make_error_code` 转成 `error_code`。

<!-- compile -->

```cpp
#include <iostream>
#include <system_error>

int main() {
  std::error_code ok;
  std::error_code missing =
      std::make_error_code(std::errc::no_such_file_or_directory);

  std::cout << static_cast<bool>(ok) << ' ' << static_cast<bool>(missing)
            << '\n';
  std::cout << missing.value() << ' ' << missing.message() << '\n';
}
```

输出：

```text
0 1
2 No such file or directory
```

完整版本见 [`examples/cpp11/error_code.cpp`](../../examples/cpp11/error_code.cpp)，六个场景的输出为：

```text
[1] ok = 0, missing = 2, message = No such file or directory, category = generic
[2] opened = 0, failed = 1, message = No such file or directory
[3] code message = No such file or directory, matches = 1
[4] empty = parse: empty input, overflow = value overflow
[5] same value = 1, equal = 0
[6] parse(5) ok = 1, parse(-1) message = value overflow
```

场景 1 展示成功与失败的基本表示：默认构造的值是 0，`errc` 转换出的值带有通用类别和可读消息。场景 2 把 `errno` 转成 `error_code`，文件打开失败后不再依赖全局错误状态。场景 3 用 `std::system_error` 包装同一个错误，捕获后仍能取回原始 `error_code` 并和 `errc` 比较。场景 4 定义自定义类别，把项目内枚举映射成独立错误空间。场景 5 说明数值相同但类别不同的两个错误并不相等。场景 6 让函数直接返回 `error_code`，调用方只检查布尔值就能区分成功与失败。

类别名 `generic` 来自 `std::generic_category()`；标准还提供 `std::system_category()` 表示操作系统错误，以及 `std::iostream_category()` 表示流错误。错误消息通过 `message()` 按类别生成，调用方不需要自己维护错误码到文本的映射表。

## 进阶用法

**`error_condition` 与 `errc` 比较。** `error_code` 表示“某个具体实现产生的错误”，`error_condition` 表示“可移植的错误条件”。多数标准库接口允许把 `error_code` 与 `std::errc` 直接比较，库可以把自己平台上的错误值映射到通用条件，调用方只关心“是不是找不到文件”，不必知道底层是 `ENOENT` 还是其他值。

**自定义错误类别。** 继承 `std::error_category`，实现 `name()` 与 `message(int)`，再用 `std::error_code{value, category}` 构造。若希望枚举能隐式转成 `error_code`，特化 `std::is_error_code_enum` 并提供 `make_error_code`；也可以像示例那样直接调用转换函数，避免污染 `std` 命名空间。

**把 `errno` 桥接进来。** C 接口通常返回 `-1` 并设置 `errno`。调用前把 `errno` 清零，失败后构造 `std::error_code{errno, std::generic_category()}`，就能把 C 风格错误纳入同一套检查流程。

**抛出时用 `system_error`。** `std::system_error(code)` 或 `std::system_error(code, "context")` 构造异常，`what()` 包含上下文与消息，`code()` 返回原始 `error_code`。这样函数可以在“返回错误”和“必须抛出”的接口之间转换而不丢失信息。

**无异常接口。** 文件系统、网络和许多 I/O 库提供接受 `std::error_code&` 的重载。它们不抛异常，把结果写入输出参数；调用方先看返回值，再检查 `ec` 是否被置位。这两步必须都做，因为有些 API 用 `false` 表示“条件不成立”，并不代表发生错误。

## 真实项目证据

| 项目           | 位置                                                                                                                                                                                 | 用法                                                  | 为什么典型                                   |
| -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------- | -------------------------------------------- |
| mold           | [`src/subprocess-unix.cc#L66-L72`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/subprocess-unix.cc#L66-L72)                                      | 多次探测候选路径，用输出型 `error_code` 接收失败信息  | 成功路径不抛异常，失败原因保留到最终诊断     |
| drogon         | [`lib/src/HttpAppFrameworkImpl.cc#L1178-L1186`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/src/HttpAppFrameworkImpl.cc#L1178-L1186) | 创建日志目录失败时检查 `fsErr` 并终止进程             | 已存在目录与真正失败需要区分，仅看返回值不够 |
| NVIDIA/stdexec | [`include/stdexec/__detail/__task.hpp#L242-L246`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__task.hpp#L242-L246)     | 把任务完成通知里的 `error_code` 包装成 `system_error` | 错误码是通用载荷，只在必须抛异常时再升级     |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处覆盖探测、启动期失败处理和异步错误升级，都不是测试代码。

## 陷阱与边界

**只比较 `value()` 不够。** 不同类别可能产生相同整数。比较两个 `error_code` 时类别和值同时参与；需要判断具体错误时直接与 `errc` 或同一类别构造的值比较。

**`operator bool` 的语义是“有错误”。** 默认构造为成功，非零值为失败。不要写成 `if (!ec) { /* 失败 */ }`，这会把成功分支当成错误分支。

**先清零 `errno` 再调用 C 函数。** C 函数失败时设置 `errno`，成功时不一定清零；不重置就可能把上一次的残留错误当成这次的结果。

**`message()` 可能分配内存。** 它按需生成本地化文本，热路径上只判断布尔值或比较错误值，不要每轮都取消息字符串。

**`errc` 与 `system_category` 不是一回事。** `std::make_error_code(std::errc::...)` 产生通用类别；操作系统 API 常返回系统类别。直接用整数构造 `error_code` 时，必须选对类别。

**`error_code` 很小，但不要吞掉上下文。** 它只携带值与类别，不携带发生位置；跨层传播时应在边界补充操作名、路径等信息，再转换成用户可见的诊断。

**自定义类别要有稳定生命周期。** 类别对象通常返回静态实例的引用，`error_code` 保存的是指针，不要让类别在 `error_code` 仍被使用时析构。

## 库联动

- **`std::filesystem`**：`exists`、`create_directories` 等都有接受 `std::error_code&` 的 `noexcept` 重载。
- **`std::system_error`**：把 `error_code` 包成异常，保留类别与消息。
- **`std::errc` / `error_condition`**：提供可移植的错误判断，屏蔽平台差异。
- **网络与异步库**：Asio 风格回调常以 `std::error_code` 作为完成状态，成功时为空。
- **C++23 `std::expected`**：需要把错误值作为返回类型的一部分时，用 `error_code` 作为 `E` 可以兼顾轻量与可读性。

## 横向对比

| 方案                 | 携带类别 | 需要异常 | 强制检查           | 主要代价                             |
| -------------------- | -------- | -------- | ------------------ | ------------------------------------ |
| `std::error_code`    | 是       | 否       | 否，仍可忽略返回值 | 需要调用方主动检查；消息生成可能分配 |
| `std::system_error`  | 是       | 是       | 否，可捕获也可穿透 | 部分项目禁用异常                     |
| `std::expected<T,E>` | 是       | 否       | 类型可见，仍可忽略 | C++23 才有；错误类型进入公开 API     |
| 整型返回码           | 否       | 否       | 否                 | 正常值与错误码混用，语义靠约定       |
| `errno`              | 否       | 否       | 否                 | 全局状态，多线程与嵌套调用都容易出错 |

`std::error_code` 由 N2241 进入 C++11，和 `std::system_error`、`std::error_condition` 一起构成标准错误报告设施。它不取代异常，而是为“错误是常态、异常不可用或代价太高”的接口提供统一表示。
