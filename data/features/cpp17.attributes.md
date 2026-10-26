---
id: cpp17.attributes
title: 标准属性
standard: C++17
kind: language
category: [语言机制, 诊断]
difficulty: 2
status: final
tier: B
compilers: {gcc: "7", clang: "3.9", msvc: "19.11"}
tags: [nodiscard, maybe_unused, fallthrough, deprecated, 警告]
alternatives: [编译器扩展属性, 文档约定, 静态分析]
proposal: P0189R1
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "在协程 awaiter 类型上标注 [[nodiscard]]，提醒调用方必须消费返回值"
    evidence: [ev-drogon-nodiscard-awaiter-001]
  - id: c2
    statement: "在虚接口上标注 [[deprecated]] 并给出替代函数名"
    evidence: [ev-drogon-deprecated-create-db-client-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

“这个返回值不能丢”“这个分支故意贯穿”“这个参数暂时不用”“这个接口准备废弃”过去只能写在注释里，或者依赖编译器的私有扩展。注释不会进入编译器，私有扩展换编译器就失效。

C++11 统一了 `[[...]]` 属性语法，C++17 通过 P0189R1、P0212R1、P0188R1 补齐了 `[[nodiscard]]`、`[[maybe_unused]]`、`[[fallthrough]]` 等标准属性。它们把意图交给编译器和静态分析工具，由工具在编译期发出诊断。

## 基础用法

<!-- compile -->

```cpp
#include <iostream>

[[nodiscard]] int compute(int value) { return value * 2; }

[[deprecated("use compute instead")]] int compute_old(int value) {
  return value;
}

int classify(int value) {
  int result = 0;
  switch (value) {
  case 0:
    result = 1;
    [[fallthrough]];
  case 1:
    result += 10;
    break;
  default:
    break;
  }
  return result;
}

int main() {
  [[maybe_unused]] int cached = compute(3);
  std::cout << classify(0) << '\n';
}
```

`[[nodiscard]]` 让“丢弃 `compute` 的返回值”触发警告；`[[maybe_unused]]` 说明变量是有意保留但当前未读取；`[[fallthrough]]` 明确表示 `case 0` 故意继续执行 `case 1`；`[[deprecated]]` 把迁移方向写进诊断信息。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                     | 用法                                         | 为什么典型                                             |
| ------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------- | ------------------------------------------------------ |
| Drogon | [`orm_lib/inc/drogon/orm/DbClient.h#L59-L64`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/inc/drogon/orm/DbClient.h#L59-L64)         | 在协程 awaiter 类型上标注 `[[nodiscard]]`    | awaiter 必须被 `co_await` 消费，丢弃临时对象通常是错误 |
| Drogon | [`lib/inc/drogon/HttpAppFramework.h#L1511-L1516`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/HttpAppFramework.h#L1511-L1516) | 在旧虚接口上标注 `[[deprecated]]` 与替代名称 | 保留源码兼容性，同时在编译期把调用方引向新 API         |

两条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。两者都位于生产头文件，而不是测试或示例。

## 陷阱与边界

**属性不是运行时检查。** `[[nodiscard]]` 只产生警告，不会阻止程序运行；`[[deprecated]]` 也只产生诊断，旧接口仍然可以调用。需要运行时保证时，用断言、错误码或契约设施。

**实现可以忽略未知属性。** 标准允许编译器忽略它不认识的属性；不同编译器对同一属性的诊断范围也不同。不要把属性当成可移植的行为约束。

**`[[fallthrough]]` 只能放在 switch 里。** 它必须出现在下一个 `case` 或 `default` 标签之前，表示上一个分支有意贯穿；放在普通语句块里没有意义，编译器可能直接报错。

**`[[maybe_unused]]` 不是消除死代码的工具。** 它适合参数、调试变量、条件编译保留的实体。把真实未使用变量都标上，只会掩盖重构遗留问题。

**`[[nodiscard]]` 的位置要看清。** 标在函数上约束该函数返回值；标在类上约束返回该类类型的函数。确实要丢弃时，写 `(void)compute(3);` 明确表达意图，而不是关掉警告。

**`[[deprecated]]` 的消息是实现定义的。** 消息字符串不保证原样出现在所有编译器的诊断里；迁移说明还应写进文档和发布说明，不能只依赖属性。

**属性列表可以合并，但顺序与位置有规则。** 多个属性写在同一个 `[[...]]` 里，或分成多个属性说明符；声明、参数、类型上的可用位置由标准规定，放错位置可能被忽略或拒绝。

## 横向对比

| 方案               | 可移植性   | 编译期诊断     | 运行时保证 | 主要代价                       |
| ------------------ | ---------- | -------------- | ---------- | ------------------------------ |
| 标准属性 `[[...]]` | 跨编译器   | 是（范围不一） | 否         | 只能表达有限意图               |
| 编译器扩展属性     | 绑定实现   | 是             | 否         | 换编译器或平台需要条件编译     |
| 注释与文档约定     | 高         | 否             | 否         | 完全依赖人和审查               |
| 静态分析规则       | 工具相关   | 是             | 否         | 需要额外工具链与规则维护       |
| 断言、错误码、契约 | 标准或提案 | 部分           | 是         | 有运行时开销，契约支持仍在演进 |

跨标准演进：C++11 引入统一属性语法；C++14 加入 `[[deprecated]]`；C++17 加入 `[[nodiscard]]`、`[[maybe_unused]]`、`[[fallthrough]]`；C++20 增加 `[[likely]]`、`[[unlikely]]`、`[[no_unique_address]]` 等。这些属性始终只负责编译期诊断，不提供运行期检查。
