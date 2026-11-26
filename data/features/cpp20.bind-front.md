---
id: cpp20.bind-front
title: std::bind_front
standard: C++20
kind: library
category: [函数式, 泛型编程]
difficulty: 2
status: final
tier: A
compilers: {gcc: "9", clang: "13", msvc: "19.25"}
tags: [bind_front, 部分应用, 回调, 参数绑定]
alternatives: [lambda, std::bind, std::invoke, std::bind_back]
proposal: P0356R5
last_verified: "2026-11-20"
claims:
  - id: c1
    statement: "为头字段格式化回调预绑定输入字符串视图"
    evidence: [ev-nghttp2-bind-front-capitalizer-001]
  - id: c2
    statement: "把响应体长度绑定到整数格式化回调"
    evidence: [ev-nghttp2-bind-front-content-length-002]
  - id: c3
    statement: "通过特性测试宏选择标准 bind_front 或兼容实现"
    evidence: [ev-abseil-bind-front-compatibility-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

回调常有一部分参数在注册时就已知，其余参数在调用时由框架提供。例如缓冲区追加接口知道输出位置，格式化函数的输入值却由调用方决定。`std::bind_front(f, a)` 保存 `f` 和第一个参数 `a`，调用时把剩余参数接到后面。

以格式化整数为例，原函数接受 `(value, output)`。当缓冲区回调只传入 `output` 时，可先绑定 `value`，使签名正好匹配。若还要调整参数顺序、转换值或带上条件，通常直接写 lambda。

## 基础用法

在 `<functional>` 中引入 `std::bind_front`。下面的 `from_ten(3)` 调用目标函数时，参数顺序是 `10, 3`。

<!-- compile -->

```cpp
#include <functional>
#include <iostream>

int main() {
  auto from_ten =
      std::bind_front([](int left, int right) { return left - right; }, 10);
  std::cout << from_ten(3) << '\n';
}
```

最小示例输出 `7`。完整示例从普通值绑定扩展到引用、所有权和成员函数，共六个场景。[bind_front.cpp](../../examples/cpp20/bind_front.cpp)

```text
[1] 10 - 3 = 7
[2] updated total = 9
[3] consumed value = 42
[4] bound offset + 3 = 13, current offset = 20
[5] counter = 6
[6] combined digits = 123
```

场景 1 先绑定减法左操作数，调用时再传入右操作数。场景 2 通过 `std::ref` 修改原变量。场景 3 把 `unique_ptr` 存入包装器，调用时移交给目标函数。

场景 4 在绑定后把原变量改为 20，包装器仍用绑定时保存的 10。场景 5 绑定成员函数和对象地址，再传入增量。场景 6 同时固定前两个参数，调用时只补第三个。

## 进阶用法

**先决定复制还是借用。** 绑定器保存的是参数经 `std::decay` 后的值。场景 4 的左值整数会被复制。需要修改原对象时，可以传 `std::ref(obj)`。若保存的是 `string_view`，复制的仍是视图，字符串数据保持借用关系。

**包装器的值类别决定如何传参。** 以普通左值调用时，保存的参数作为左值传递。场景 3 的目标按值接收 `unique_ptr`，需要用 `std::move(wrapper)()`，让包装器将保存的指针作为右值传递。仅创建绑定器还没有执行目标函数。

**绑定成员函数时要考虑接收对象。** 调用按 `std::invoke` 的规则进行。场景 5 固定了 `Counter` 的地址，回调每次执行都修改同一个实例。若直接传普通对象左值，绑定器保存其副本，修改就发生在副本上。

## 真实项目证据

| 项目              | 固定源码                                                                                                                                                              | 用法                                         | 为什么典型                                             |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------- | ------------------------------------------------------ |
| nghttp2/nghttp2   | [`src/http2.cc#L299-L310`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/http2.cc#L299-L310)                                   | 为头字段格式化回调预绑定输入字符串视图       | 缓冲区追加接口稍后提供输出位置，回调只需预先保存输入。 |
| nghttp2/nghttp2   | [`src/shrpx_https_upstream.cc#L1118-L1120`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/shrpx_https_upstream.cc#L1118-L1120) | 把响应体长度绑定到整数格式化回调             | 待格式化的值已知，输出位置由缓冲区提供。               |
| abseil/abseil-cpp | [`absl/functional/bind_front.h#L186-L195`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/functional/bind_front.h#L186-L195) | 通过特性测试宏选择标准 bind_front 或兼容实现 | 在标准库支持时复用其实现，同时维持同一套库接口。       |

三条证据均来自固定 commit 的生产路径，现为 `manual` 级。

## 陷阱与边界

- `std::ref`、裸指针和 `std::string_view` 都可能借用外部对象。保存回调前要保证被引用对象或数据能活到调用完成。
- 绑定右值会将它存进包装器。若后续调用移出了保存的资源，重复调用前要检查目标函数能否处理被移走的状态。
- 参数按顺序追加，无法用占位符调整位置。需要重排、按条件决定参数或执行额外步骤时，lambda 更适合。
- 遇到重载函数名时，先用明确的函数指针类型选定重载，或用 lambda 在调用处完成重载选择。
- 包装器类型由实现提供，通常用 `auto` 接收。如需类型擦除，先确认包装器符合目标接口对拷贝或移动的要求。

## 库联动

- [std::invoke](cpp17.invoke-apply.md) 统一处理自由函数、函数对象和成员指针。`bind_front` 把部分参数的准备提前到创建包装器时。
- [std::reference_wrapper](cpp11.reference-wrapper.md) 可显式表达借用关系。
- [std::function](cpp11.function.md) 可接收满足其可复制要求的包装器。绑定了 `unique_ptr` 的包装器只能移动，可直接使用具体类型，或在支持 C++23 时用 `std::move_only_function`。
- Abseil 的 `absl::bind_front` 在标准设施可用时直接引入它，供跨标准版本的库继续使用原接口。

## 横向对比

| 写法                    | 适用场景                         |
| ----------------------- | -------------------------------- |
| `std::bind_front(f, a)` | 固定左侧参数，按顺序接收剩余参数 |
| lambda                  | 需要重排、计算参数或控制捕获方式 |
| `std::bind`             | 维护使用占位符的旧接口           |
| `std::bind_back(f, a)`  | 固定右侧参数，C++23 起可用       |

标准依据见 [P0356R5](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0356r5.html) 和 [调用包装器规则](https://eel.is/c++draft/func.bind.partial)。
