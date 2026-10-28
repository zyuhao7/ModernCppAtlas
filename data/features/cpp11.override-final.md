---
id: cpp11.override-final
title: override 与 final
standard: C++11
kind: language
category: [面向对象, 类型系统]
difficulty: 2
status: final
tier: A
compilers: {gcc: "4.7", clang: "2.9", msvc: "14.0"}
tags: [override, final, 虚函数, 继承, 覆写]
alternatives: [注释约定, 纯虚接口, 非虚接口 NVI, 组合]
proposal: N2928
last_verified: "2026-10-27"
claims:
  - id: c1
    statement: "在格式化器派生类上同时使用 final 与 override 固定继承契约"
    evidence: [ev-spdlog-override-final-formatter-001]
  - id: c2
    statement: "用 final 标记异步 logger 禁止进一步派生"
    evidence: [ev-spdlog-final-async-logger-002]
  - id: c3
    statement: "在装饰器派生类上用 override 覆写虚函数与析构函数"
    evidence: [ev-drogon-override-ratelimiter-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

覆写虚函数时，签名必须与基类完全一致。漏写 `const`、参数类型差一个引用、返回类型不兼容，都会让派生类函数变成新的重载而不是覆写。编译器不会报错，运行时却仍然调用基类版本——这类错误在大型继承体系里极难排查。另一方面，库作者有时需要明确禁止某个类被继续派生，或禁止某个虚函数被再次覆写，但语言原本没有表达手段。

`override` 让编译器检查“这个函数确实覆写了基类虚函数”，签名不匹配立即报错。`final` 放在类或虚函数后，禁止继续派生或覆写。两者都是上下文关键字，不改 ABI，也不产生运行期开销。标准提案为 N2928（`override`）与 N3206（`final`）。

## 基础用法

派生类函数末尾写 `override`；类名或函数声明末尾写 `final`。

<!-- compile -->

```cpp
#include <iostream>
#include <memory>
#include <vector>

class Shape {
 public:
  virtual ~Shape() = default;
  virtual double area() const = 0;
};

class Square final : public Shape {
 public:
  explicit Square(double side) : side_(side) {}

  double area() const override { return side_ * side_; }

 private:
  double side_;
};

int main() {
  std::vector<std::unique_ptr<Shape>> shapes;
  shapes.push_back(std::unique_ptr<Shape>(new Square(3.0)));
  std::cout << shapes.front()->area() << '\n';
}
```

输出 `9`。`Square` 用 `final` 禁止派生，`area` 用 `override` 让签名错误在编译期暴露。完整版本见 [`examples/cpp11/override_final.cpp`](../../examples/cpp11/override_final.cpp)。

## 进阶用法

`override` 只对虚函数合法。若基类函数不是虚函数，或参数、`const`、引用限定符不一致，编译器会直接报错。它同时替代了 `virtual` 关键字的位置：写了 `override` 就不必再写 `virtual`，语义仍然是虚函数。析构函数也可以标 `override`，尤其是多态基类的析构应声明为 `virtual`，派生类再用 `override = default` 明确覆写。

`final` 有两个位置。类名后的 `final` 禁止任何人继承该类；虚函数声明后的 `final` 禁止派生类继续覆写该函数，但类仍可被继承。两者都只影响编译期检查，不会改变对象布局或调用约定。

把 `final` 用在虚函数上还能给编译器更多优化空间：调用点若确定动态类型，编译器可以去除虚表间接调用。标准库和大型框架常用 `final` 封闭实现类，减少误用和优化障碍。注意 `final` 不是访问控制，它不阻止实例化或组合，只阻止继承与覆写。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                                  | 用法                                     | 为什么典型                                 |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------- | ------------------------------------------ |
| spdlog | [`include/spdlog/pattern_formatter-inl.h#L103-L113`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/pattern_formatter-inl.h#L103-L113) | 派生格式化器同时标 `final` 与 `override` | 实现类不再扩展，覆写签名由编译器保证       |
| spdlog | [`include/spdlog/async_logger.h#L33-L34`](https://github.com/gabime/spdlog/blob/79524ddd08a4ec981b7fea76afd08ee05f83755d/include/spdlog/async_logger.h#L33-L34)                       | 异步 logger 类标 `final`                 | 公共 API 明确继承层级到此为止              |
| drogon | [`lib/inc/drogon/RateLimiter.h#L56-L69`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/lib/inc/drogon/RateLimiter.h#L56-L69)                | 装饰器类用 `override` 覆写虚函数与析构   | 基类签名变化时立即暴露，默认析构保持虚语义 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖实现类封闭、公共类封闭与装饰器覆写三种用途。

## 陷阱与边界

**`override` 不能修饰非虚函数。** 基类函数没有 `virtual` 时，派生类写 `override` 会编译失败；这通常说明基类接口设计有问题。

**签名必须逐字匹配。** 参数类型、`const`、引用限定符、异常规格都会影响覆写关系；返回类型只允许协变。`override` 的价值正是把这些差异变成编译错误。

**`final` 类不能被继承。** 测试替身、mock 和扩展点如果需要派生，不能把 `final` 加在类上；可以只对具体虚函数加 `final`。

**析构函数也要虚。** 通过基类指针删除派生对象时，基类析构必须是 `virtual`；`override` 只能检查覆写，不能弥补基类析构非虚的错误。

**不要用 `final` 掩盖设计问题。** 它适合封闭实现或明确不再扩展的接口；如果只是因为“暂时没人继承”就加 `final`，后续扩展会受阻。

**ABI 与二进制兼容。** `override` 和 `final` 是编译期说明符，不改变布局；但给已有虚函数追加 `final` 可能影响内联与去虚化，跨版本库升级时仍需回归测试。

## 库联动

- **多态基类**：接口类通常声明纯虚函数和虚析构，实现类用 `override` 明确覆写。
- **标准库**：`std::exception` 派生类、自定义迭代器与分配器常需要覆写虚函数，`override` 可避免签名偏差。
- **测试框架**：mock 类覆写接口时用 `override`，接口变更会在编译期暴露所有受影响替身。
- **spdlog / drogon**：日志格式化器与限流装饰器都用这两个说明符固定继承契约。

## 横向对比

| 方案               | 签名错误检查   | 禁止继续派生 | 运行期开销   | 主要限制                 |
| ------------------ | -------------- | ------------ | ------------ | ------------------------ |
| 仅写 `virtual`     | 否             | 否           | 有虚调用     | 拼写错误静默变成新函数   |
| `override`         | 是             | 否           | 无额外开销   | 只能用于虚函数           |
| `final`            | 否             | 是           | 可能利于优化 | 过度使用会阻碍扩展       |
| `override`+`final` | 是             | 是           | 可能利于优化 | 同时限制扩展与覆写       |
| 纯虚接口 + 组合    | 编译期强制实现 | 视设计而定   | 有虚调用     | 继承层级固定，组合更灵活 |

跨标准演进：C++11 通过 N2928 引入 `override`、通过 N3206 引入 `final`（N3272 补充细节）；二者此后未再变化，C++20 的 `concept` 与 `requires` 用于约束模板，不替代虚函数覆写检查。
