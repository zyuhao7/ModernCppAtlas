---
id: cpp11.inline-namespaces
title: 内联命名空间（inline namespace）
standard: C++11
kind: language
category: [语法, 语言机制]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.4", clang: "2.9", msvc: "19.0"}
tags: [inline namespace, ABI 版本, ADL, 库设计, 命名空间]
alternatives: [嵌套命名空间加 using 声明, 宏改写符号名, 符号版本脚本, 匿名命名空间]
proposal: N2535
last_verified: "2026-11-25"
claims:
  - id: c1
    statement: "API 宏展开为外层 namespace fmt 加内联 namespace v12"
    evidence: [ev-fmt-inline-namespace-001]
  - id: c2
    statement: "用配置宏选择展开为空或展开为 inline namespace，注入可配置的 ABI 标签"
    evidence: [ev-abseil-inline-namespace-002]
  - id: c3
    statement: "用嵌套内联命名空间 literals/json_literals 暴露用户定义字面量"
    evidence: [ev-nlohmann-inline-namespace-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

库需要在源码名字不变的前提下替换二进制实现：用户始终写 `fmt::format`，但不同版本的头文件可以把符号放进不同命名空间，避免链接到旧 ABI 的定义。C++11 之前只能用宏改符号名，或者让用户书写带版本段的完整名字；前者污染全局命名空间，后者把实现细节写进用户代码。

C++11 的 `inline namespace`（N2535）在语义上把内层命名空间的成员同时放进外层：`namespace fmt { inline namespace v12 { ... } }` 里的 `format` 既可写作 `fmt::v12::format`，也可直接写作 `fmt::format`。版本的区分留给库，源码书写方式保持不变。

## 基础用法

内联命名空间的成员在使用方看来属于外层命名空间，同时内层名字仍然可以被显式限定。查找、重载决议和模板实参推导都把它们当作外层成员处理。

<!-- compile -->

```cpp
namespace atlas {
inline namespace v2 {
constexpr int version = 2;
struct Widget {
  int size;
};
}  // namespace v2
}  // namespace atlas

int main() {
  atlas::Widget widget{4};
  return atlas::version + widget.size;
}
```

完整示例见 [`examples/cpp11/inline_namespaces.cpp`](../../examples/cpp11/inline_namespaces.cpp)，输出为：

```text
[1] atlas::version = 2
[2] atlas::v2::version = 2
[3] sizeof(atlas::Widget) = 4
[4] atlas::Traits<int>::id = 7
[5] ADL describe: widget 4 bytes
```

场景 1 与 2 验证两种写法都能找到同一个 `version`；场景 3 通过外层类型名访问内层结构体。场景 4 的显式特化写在 `atlas::v2::Traits<int>`，说明特化必须写出真实的命名空间。场景 5 依赖 ADL：参数类型位于内联命名空间，非限定调用仍能找到 `atlas::v2::describe`。

## 进阶用法

**外层名字是稳定接口。** 库把实现放进内联命名空间，只把外层 `namespace fmt` 当作文档和用户代码的契约。更换 ABI 版本时改内联命名空间名，用户代码一行不动；`fmt` 用 `FMT_BEGIN_NAMESPACE` 宏把这一层封装起来。

**ABI 标签可以配置。** 内联命名空间的名可以是宏展开的标识符。abseil 用 `ABSL_NAMESPACE_BEGIN` 在需要时展开为 `inline namespace <ABI 标签> {`，在不需要时展开为空；同一份头文件因此能适配不同发布策略。

**显式特化和前置声明要写全名。** `template <> struct atlas::Traits<int>` 不会命中内联命名空间里的模板，必须写成 `atlas::v2::Traits<int>`。前置声明同理；写错层级会得到“特化未声明”或另一个独立模板。

**ADL 会看到内联命名空间。** 实参类型属于内层命名空间时，关联命名空间包含内层与其外层，因此非限定调用能找到内层函数。这也是定制点对象常放进内联命名空间的原因之一。

**嵌套内联命名空间可以分层。** 如 `inline namespace literals { inline namespace json_literals { ... } }`，字面量运算符既可从最内层限定访问，也能从 `nlohmann` 外层直接找到，同时避免污染全局命名空间。

**它不改变 ABI 本身。** 内联命名空间改变的是名字查找和符号修饰名；是否破坏二进制兼容仍取决于类型布局、函数签名和调用约定。版本段只是让新旧符号可以共存。

## 真实项目证据

| 项目              | 固定源码                                                                                                                                                        | 用法                                                       | 为什么典型                                     |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------- | ---------------------------------------------- |
| fmtlib/fmt        | [`include/fmt/base.h#L260-L267`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/base.h#L260-L267)                      | API 宏展开为 `namespace fmt` 加 `inline namespace v12`     | 整个公共 API 通过内联命名空间获得 ABI 版本段   |
| abseil/abseil-cpp | [`absl/base/config.h#L175-L182`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/base/config.h#L175-L182)               | 配置宏选择展开为空或展开为 `inline namespace`              | ABI 标签可配置，调用方源码不受影响             |
| nlohmann/json     | [`include/nlohmann/json.hpp#L5207-L5219`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/json.hpp#L5207-L5219) | 嵌套内联命名空间 `literals/json_literals` 暴露字面量运算符 | 字面量隔离在独立命名空间，同时可从外层直接使用 |

三条证据来自固定 commit 的生产路径，现为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **同一父命名空间只能有一个同名内联命名空间。** 两个内联命名空间都提供同名成员时，外层查找会产生歧义；版本切换应替换内联命名空间，而不是并列保留。
- **特化、前置声明要写真实层级。** 内联只是“外层可见”，不是“外层定义”；显式特化和友元声明必须指向内层命名空间。
- **不要由用户代码重新打开内层命名空间。** 用户应依赖外层名字；直接写 `namespace fmt::v12` 的代码在版本升级后会失效。
- **`using namespace` 的影响被放大。** 内联命名空间的成员本来就在外层可见，再写 `using namespace` 只会引入更多名字，容易与其它库冲突。
- **不用于隐藏实现。** 内联命名空间的成员仍是外部可见符号；需要内部链接时用匿名命名空间或 `static`。
- **ABI 安全不能只靠命名空间。** 类型大小、内联函数和异常规范变化仍会破坏兼容性；版本段只解决符号共存。
- **工具链下限。** GCC 4.4、Clang 2.9、MSVC 19.0 起支持；极老的 MSVC 需要宏改写符号名。

## 库联动

- [用户定义字面量](cpp11.user-defined-literals.md)：字面量运算符常放在内联命名空间里，避免污染外层。
- [别名模板](cpp11.alias-templates.md)：版本化命名空间里的类型别名，让外层名字保持稳定。
- [内联变量](cpp17.inline-variables.md)：C++17 的内联变量常与内联命名空间一起做版本化常量。
- [定制点对象](cpp20.bind-front.md)：函数对象放进内联命名空间，既参与 ADL 又带 ABI 标签。
- **符号版本脚本**：链接器层面的版本控制，与内联命名空间互补，处理 C 符号和导出表。
- **模块**：C++20 模块的导出边界会改变名字可见性，但内联命名空间仍用于模块内部的 ABI 分层。

## 横向对比

| 方案                      | 引入版本 | 源码写法   | 符号修饰名       | 主要限制                         |
| ------------------------- | -------- | ---------- | ---------------- | -------------------------------- |
| 内联命名空间              | C++11    | 外层名     | 含内层命名空间名 | 特化与前置声明要写全名           |
| 嵌套命名空间加 using 声明 | C++98    | 需要 using | 含内层命名空间名 | using 要出现在每个作用域         |
| 宏改写符号名              | C++98    | 宏名       | 由宏决定         | 污染全局名字，调试困难           |
| 匿名命名空间              | C++98    | 外层名     | 内部链接         | 每个翻译单元一份，不能跨单元共享 |
| 链接器版本脚本            | 工具链   | 源码不变   | 由脚本控制       | 平台相关，只处理导出符号         |

内联命名空间用标准语法解决了“源码名稳定、符号名可变”的库设计问题。它适合有 ABI 版本承诺的库、字面量命名空间和定制点对象；普通应用代码很少需要，误用最容易出现在显式特化和前置声明上。
