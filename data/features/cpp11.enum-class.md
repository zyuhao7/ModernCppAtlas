---
id: cpp11.enum-class
title: 强类型枚举（enum class）
standard: C++11
kind: language
category: [类型系统, 语法]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.4", clang: "2.9", msvc: "17.0"}
tags: [enum class, 强类型枚举, 作用域, 底层类型, 类型安全]
alternatives: [无作用域枚举, constexpr 常量, 类内静态常量, 位标志枚举]
proposal: N2347
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "用 enum class 表示文件类型，并在 switch 中使用限定名分支"
    evidence: [ev-mold-enum-class-filetype-001]
  - id: c2
    statement: "用 enum class 区分 WebSocket 帧类型，避免与其它枚举名冲突"
    evidence: [ev-drogon-enum-class-ws-type-002]
  - id: c3
    statement: "用 enum class 表示异步消息种类并驱动线程池分派"
    evidence: [ev-spdlog-enum-class-async-msg-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

传统 `enum` 的枚举值直接进入外层作用域。两个库都定义 `None` 就会冲突；枚举值还能隐式转换成整数，和任意数字比较、参与算术运算，拼写错误也不会被发现。底层类型由实现选择，跨平台序列化时大小不确定；前向声明也不可行。

`enum class` 把枚举值关进自己的作用域，禁止隐式转换，并允许显式指定底层类型。要取值必须写 `Type::value`，要转成整数必须显式 `static_cast`。标准提案为 N2347。

## 基础用法

定义时在 `enum` 后写 `class`。使用枚举值要带类型名。

<!-- compile -->

```cpp
#include <iostream>

enum class Color { red, green, blue };
enum class Status { ok = 200, not_found = 404 };

int main() {
  Color color = Color::green;
  Status status = Status::not_found;
  std::cout << static_cast<int>(color) << ' ' << static_cast<int>(status)
            << '\n';
}
```

输出 `1 404`。`Color::green` 与 `Status::not_found` 不会泄漏到外层作用域，也不能直接和整数比较。完整版本见 [`examples/cpp11/enum_class.cpp`](../../examples/cpp11/enum_class.cpp)。

## 进阶用法

底层类型写在枚举名之后：`enum class Status : std::uint16_t { ... };`。这固定了存储宽度和取值范围，适合网络协议、文件格式与跨语言 ABI。指定底层类型后还可以前向声明 `enum class Status : std::uint16_t;`，在不完整类型上定义指针或引用，减少头文件依赖。

`enum class` 不提供隐式算术和位运算。需要位标志时，要么在枚举外重载运算符，要么改用普通的无作用域枚举或专门的 flags 类型。不要为了省一次 `static_cast` 而退回无作用域枚举，类型安全的价值通常更大。

C++20 的 `using enum Color;` 可以把某个枚举的成员引入当前作用域，减少重复前缀；`std::to_underlying`（C++23）提供比 `static_cast` 更明确的整数转换。C++17 起，`switch` 覆盖不完整时主流编译器会给出警告，配合 `-Wswitch` 可以及早发现漏分支。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                      | 用法                                       | 为什么典型                           |
| ------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------ | ------------------------------------ |
| mold   | [`src/filetype.cc#L174-L182`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/filetype.cc#L174-L182)                                                     | 用限定名做 `switch` 分支并比较文件类型     | 枚举值与解析逻辑绑定，避免整数魔法值 |
| drogon | [`lib/src/WebSocketConnectionImpl.cc#L45-L53`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/src/WebSocketConnectionImpl.cc#L45-L53)        | 用 `enum class` 区分帧类型并映射到操作码   | 帧类型名称不会与其它枚举冲突         |
| spdlog | [`include/spdlog/details/thread_pool-inl.h#L102-L114`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/details/thread_pool-inl.h#L102-L114) | 用 `enum class` 表示消息种类并驱动后台分派 | 消息类型需要限定作用域且分支完整     |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖类型判断、协议映射与任务分派三种用途。

## 陷阱与边界

**没有隐式整数转换。** 作为数组下标、传给 C 接口或做算术时都要显式 `static_cast`。这是有意的安全设计，不要用宏或全局常量绕过。

**不提供位运算。** `a | b` 无法直接编译；需要组合标志时，显式重载运算符或改用整型位掩码。

**底层类型默认不固定。** 需要稳定大小或前向声明时，显式写 `: std::uint8_t` 等类型。

**枚举值仍可被显式转换。** `static_cast<Color>(99)` 合法，可能得到一个没有名字的值；解析外部输入后要校验范围。

**无作用域枚举并未废弃。** 与 C 接口交互、需要隐式整数转换或位标志时，普通 `enum` 仍可能是合适选择；关键是明确取舍。

**序列化要固定数值。** 依赖枚举顺序会在插入新值时改变已有取值；对外协议应显式写出每个枚举值。

## 库联动

- **标准库**：`std::byte`、`std::memory_order`、`std::launch`、`std::future_status` 等都用强类型枚举表达离散状态。
- **容器与算法**：枚举可作为 `std::map` 的键，也可以配合 `std::underlying_type` 取得底层类型。
- **网络与协议库**：drogon 用强类型枚举区分 WebSocket 帧类型；spdlog 用它表示异步消息种类。
- **链接器与工具链**：mold 用强类型枚举表示文件类型，避免不同格式的整数常量互相污染。

## 横向对比

| 方案             | 名称作用域 | 隐式转整数 | 可指定底层类型 | 主要代价                       |
| ---------------- | ---------- | ---------- | -------------- | ------------------------------ |
| 无作用域 `enum`  | 外层       | 是         | 是             | 名称冲突，类型不安全           |
| `enum class`     | 枚举内     | 否         | 是             | 每次转换都要显式 `static_cast` |
| `constexpr` 常量 | 所在作用域 | 是         | 不适用         | 没有独立类型，无法整体遍历     |
| 类内静态常量     | 类内       | 视类型而定 | 不适用         | 样板代码多，不能约束取值集合   |

跨标准演进：C++11 通过 N2347 引入强类型枚举；C++17 允许用花括号初始化枚举；C++20 增加 `using enum`，C++23 提供 `std::to_underlying`。
