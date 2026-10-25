---
id: cpp17.any
title: std::any
standard: C++17
kind: library
category: [类型擦除, 动态类型, 容器]
difficulty: 2
status: final
tier: A
compilers: {gcc: "7", clang: "4", msvc: "19.11"}
tags: [类型擦除, any_cast, 异构容器, RTTI]
alternatives: [void*, std::variant, 基类 + 虚函数, std::shared_ptr<void>, absl::any]
proposal: P0220R1
last_verified: "2026-10-25"
claims:
  - id: c1
    statement: "用 std::any 作为请求级键值容器的值类型"
    evidence: [ev-drogon-any-attributes-001]
  - id: c2
    statement: "会话容器按需创建 any 值，并在锁内做类型检查后调用处理器"
    evidence: [ev-drogon-any-session-modify-003]
  - id: c3
    statement: "读取前比较 typeid，再用 any_cast 指针形式取地址"
    evidence: [ev-drogon-any-view-data-typecheck-002]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

请求属性、会话数据、插件参数这类容器，需要在同一个键值表里存放不同类型的值。用 `void *` 可以放任意指针，但类型信息、所有权和析构责任全部丢失；用基类指针加虚函数，则要求所有值共享继承体系，普通 `int`、`std::string` 无法直接放入。

`std::any` 保存一个任意**可复制**类型的值，同时记录类型信息，并在销毁、复制、移动时调用正确的操作。它适合类型集合在编译期无法枚举的场景。代价是读取方必须知道具体类型，并显式用 `std::any_cast` 取回。

## 基础用法

```cpp
#include <any>
#include <iostream>
#include <string>

int main() {
  std::any value = 42;
  if (value.type() == typeid(int))
    std::cout << std::any_cast<int>(value) << '\n';

  value = std::string("atlas");
  std::cout << std::any_cast<const std::string &>(value) << '\n';
}
```

完整示例见 [`examples/cpp17/any.cpp`](../../examples/cpp17/any.cpp)：

```text
count: int 42
name: string atlas
item: user amy 3
item: int 7
item: string x
after reset: empty
amy
pointer cast 7
bad_any_cast
```

示例把 `int`、`std::string` 与自定义结构体放进同一个 `std::vector<std::any>`，用 `type()` 分派、用引用版 `any_cast` 读取大对象，并演示指针版转换与类型不符时的异常。

## 进阶用法

**指针版 `any_cast` 不抛异常。** `std::any_cast<T>(&value)` 返回 `T *`，类型不匹配或值为空时返回 `nullptr`。值版与引用版在失败时抛 `std::bad_any_cast`。需要探测时用指针版，已经确认类型时再用值版或引用版。

**只做精确类型匹配。** 存入 `int` 后不能用 `any_cast<long>` 读取，即使 `int` 可以隐式转成 `long`。同理，`std::any value = "hello";` 存的是 `const char *`，用 `any_cast<std::string>` 读取会失败。需要字符串就显式构造 `std::string`。

**值版会复制，引用版不会。** `any_cast<std::string>(value)` 返回副本；`any_cast<const std::string &>(value)` 返回引用。大对象优先用引用版，并保证 `any` 在引用使用期间存活。

**只能存放可复制类型。** `std::any` 的复制构造要求内部值可复制，因此 `std::unique_ptr<int>` 不能直接放入。需要共享所有权时存 `std::shared_ptr<int>`；需要独占所有权，改用自定义的类型擦除容器或 `std::variant` 加指针。

**空状态是合法状态。** 默认构造的 `std::any` 与调用 `reset()` 之后的 `std::any` 都不含值；`has_value()` 为假，任何值版/引用版 `any_cast` 都抛 `bad_any_cast`。读取前先检查。

**`emplace<T>` 就地替换。** `value.emplace<Widget>(args...)` 直接构造新值，避免先造临时对象再复制；返回新值的引用。

**实现可能分配堆内存。** 标准不要求小对象优化。小整数、指针可能就地保存，较大的对象通常需要堆分配；高频路径不要用 `std::any` 代替静态类型。

**依赖 RTTI。** `type()` 与 `any_cast` 依赖 `typeid`；使用 `-fno-rtti` 构建的项目无法使用 `std::any`。需要在禁用 RTTI 的环境存放异构值时，改用显式的标签联合或自定义类型擦除。

## 真实项目证据

| 项目                   | 位置                                                                                                                                                                     | 用法                                                | 为什么典型                                       |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------- | ------------------------------------------------ |
| drogonframework/drogon | [`lib/inc/drogon/Attribute.h#L61-L75`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/Attribute.h#L61-L75)       | 请求属性表使用 `map<string, any>`，普通值可隐式存入 | 框架与业务代码共享同一容器，类型在编译期不可枚举 |
| drogonframework/drogon | [`lib/inc/drogon/HttpViewData.h#L40-L53`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/HttpViewData.h#L40-L53) | 读取前比较 `typeid`，再用指针版 `any_cast` 取地址   | 精确类型匹配；类型不符时记录错误并回退到默认值   |
| drogonframework/drogon | [`lib/inc/drogon/Session.h#L108-L122`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/Session.h#L108-L122)       | 会话容器在互斥量保护下查找、校验类型并传出引用      | 跨请求异构状态；缺失时默认构造后插入             |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。drogon 的 CMake 配置要求 C++17，三个容器都把 `std::any` 用作公开 API 的值类型。

## 陷阱与边界

- **`any_cast` 不做转换。** 只匹配存储的精确类型；`int` 与 `long`、`const char *` 与 `std::string` 都不会互相兼容。
- **值版转换会复制。** 对 `std::vector`、大结构体反复调用值版 `any_cast` 会产生意外拷贝。
- **空 `any` 转换会抛异常。** 默认构造或 `reset()` 之后，先用 `has_value()` 判断，或改用指针版转换。
- **不能存移动-only 类型。** `unique_ptr`、`std::mutex` 等不可复制类型无法放入；这是 `std::any` 的类型要求，不是实现限制。
- **没有统一访问接口。** `std::any` 不提供 `visit`、比较或哈希；每个使用点都要自己枚举已知类型，类型集合一旦扩大，分派代码要同步修改。
- **指针值不拥有对象。** 存入 `T *` 只是保存地址；对象生命周期仍由调用方管理，容易产生悬垂。
- **异常路径要处理。** 值版转换与引用版转换都可能抛 `bad_any_cast`，公共 API 边界应捕获或先做类型检查。
- **性能不可忽略。** 类型擦除、可能的堆分配与 RTTI 都比静态类型慢；热路径优先用模板或 `std::variant`。

## 库联动

- **drogon**：请求属性、视图数据与会话状态都用 `std::any` 作为异构值容器；读取端通过 `typeid` 与 `any_cast` 恢复类型。
- **Abseil**：新版本把 `absl::any` 定义为 `std::any` 的别名，并废弃旧的 polyfill；使用 Abseil 的项目可以直接采用标准类型。
- **nlohmann/json**：隐式转换的约束显式排除了 `std::any`，避免把类型擦除值误当成 JSON 可序列化类型；需要序列化时先 `any_cast` 到具体类型。
- **fmt / spdlog**：标准库没有为 `std::any` 提供 formatter；日志输出前应先按类型分派，或把值转成字符串再记录。
- **与 `std::variant` 的分工**：类型集合封闭、需要穷尽检查时用 `variant`；类型集合开放、由插件或业务方在运行期决定时用 `any`。

## 横向对比

| 方案                    | 类型安全   | 类型集合     | 所有权       | 主要代价                       |
| ----------------------- | ---------- | ------------ | ------------ | ------------------------------ |
| `std::any`              | 运行期检查 | 开放         | 拥有可复制值 | 可能堆分配；读取需知道类型     |
| `void *`                | 否         | 开放         | 不拥有       | 类型与析构全部手工管理         |
| `std::variant<Ts...>`   | 编译期检查 | 封闭         | 拥有值       | 新增类型需重编译并更新访问代码 |
| 基类 + 虚函数           | 是         | 开放（继承） | 通常堆上     | 要求共同基类；间接调用         |
| `std::shared_ptr<void>` | 否         | 开放         | 共享         | 丢失类型信息；引用计数开销     |
| `absl::any`             | 运行期检查 | 开放         | 拥有可复制值 | 新版本已退化为 `std::any` 别名 |

跨标准演进：`std::any` 随 P0220R1 进入 C++17；此后接口保持稳定，没有加入访问器或比较操作。需要模式匹配或穷尽分派时，应改用 `std::variant`，或在 `any` 之上维护显式的标签与分派表。
