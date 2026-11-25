---
id: cpp20.to-address
title: std::to_address
standard: C++20
kind: library
category: [内存管理, 类型工具]
difficulty: 2
status: final
tier: A
compilers: {gcc: "8", clang: "6", msvc: "19.22"}
tags: [to_address, 裸指针, 智能指针, 连续迭代器]
alternatives: [智能指针的 get, std::addressof, std::ranges::data]
proposal: P0653R2
last_verified: "2026-11-20"
claims:
  - id: c1
    statement: "从连续迭代器取得字符串视图的起始地址"
    evidence: [ev-nghttp2-to-address-string-view-001]
  - id: c2
    statement: "在容器赋值的连续输入快路径取得字节复制源地址"
    evidence: [ev-glaze-to-address-assign-002]
  - id: c3
    statement: "在就地向量插入时取得输入范围的起始地址"
    evidence: [ev-glaze-to-address-insert-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

泛型代码有时需要把裸指针、智能指针或连续迭代器交给接收裸地址的接口。对智能指针可以调用 `get()`，但裸指针没有这个成员。用 `std::addressof(*p)` 则必须先解引用，在申请了存储但还未构造对象时不合适。`std::to_address` 为这些指针类对象提供统一的地址提取接口。

例如一个函数收到 `vector` 中间位置的迭代器，要把后面一段元素交给接收指针和长度的接口。它可以从迭代器取得起始地址，再独立计算区间长度。

## 基础用法

在 `<memory>` 中引入 `std::to_address`。取出的地址借用现有对象，生命周期由原所有者负责。

<!-- compile -->

```cpp
#include <iostream>
#include <memory>

int main() {
  auto owner = std::make_unique<int>(42);
  int* raw = std::to_address(owner);
  std::cout << *raw << '\n';
}
```

最小示例输出 `42`。完整示例演示五种取址和使用地址的场景。[to_address.cpp](../../examples/cpp20/to_address.cpp)

```text
[1] owned value = 42
[2] second value = 20
[3] null stays null = 1
[4] wrapped address matches = 1
[5] tail = 20 30
```

场景 1 从 `unique_ptr` 取出与 `get()` 一致的地址，所有权仍留在智能指针里。场景 2 从连续迭代器取得第二个元素的地址。场景 3 验证空裸指针原样返回。

场景 4 用只有 `operator->()` 的包装类型演示取址回退路径。场景 5 将场景 2 的地址和剩余长度传给 `span`，得到原向量后两个元素的视图。

## 进阶用法

**自定义指针怎样提供地址。** 裸指针重载直接返回实参。对指针类对象，若 `std::pointer_traits<Ptr>::to_address(p)` 有效，标准会调用它。否则递归处理 `p.operator->()` 的返回值。自定义分配器指针可以通过 `pointer_traits` 提供取地址方式，普通包装类型可像场景 4 一样使用 `operator->()`。

**把连续区间交给裸指针接口。** nghttp2 把起始地址和范围长度分别计算，再构造字符串视图。手里已有完整范围时，用 `std::ranges::data` 或直接构造 `span` 通常更直观。手里只有区间起点和元素数时，`to_address(first)` 加长度更合适。

**取址与复制分别判断。** glaze 的连续输入路径先检查元素类型和迭代器，再把地址交给 `memcpy`。连续性只保证布局。复制是否有效还取决于对象类型、目标空间是否足够及源目标是否重叠。对只需读取的调用，可以先用视图借用数据。

## 真实项目证据

| 项目               | 固定源码                                                                                                                                                                                             | 用法                                         | 为什么典型                                           |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------- | ---------------------------------------------------- |
| nghttp2/nghttp2    | [`src/template.h#L379-L386`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/template.h#L379-L386)                                                              | 从连续迭代器取得字符串视图的起始地址         | 借用原数据构造视图，起始位置与范围长度分别取得。     |
| stephenberry/glaze | [`include/glaze/containers/inplace_vector.hpp#L587-L593`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/containers/inplace_vector.hpp#L587-L593) | 在容器赋值的连续输入快路径取得字节复制源地址 | 地址提取与可平凡复制类型及连续迭代器的检查配合使用。 |
| stephenberry/glaze | [`include/glaze/containers/inplace_vector.hpp#L895-L903`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/containers/inplace_vector.hpp#L895-L903) | 在就地向量插入时取得输入范围的起始地址       | 先移动目标容器中的现有元素，再用输入地址复制新元素。 |

三条证据均来自固定 commit 的生产路径，现为 `manual` 级。

## 陷阱与边界

- 取地址和解引用是两回事。空裸指针可原样返回，仍不可解引用。取到尚未构造对象的存储地址后，要先构造对象才能读取其值。
- 智能指针销毁或容器发生重分配时，借出的地址可能失效。
- 链表迭代器等非连续迭代器即使能取得某个元素的地址，也不能把从该地址起的内存当成连续区间。
- 尾后迭代器即使能转换为尾后地址，也只能作为区间边界使用。它指向的位置不能读取元素。
- 裸指针重载不接受函数指针。

## 库联动

- [std::span](cpp20.span.md) 从地址和长度构造连续视图。已有连续范围时，可优先用对应的范围构造形式。
- [std::construct_at](cpp20.construct-at.md) 可在满足大小与对齐要求的存储上构造对象。自定义分配器的指针可先用 `to_address` 转成裸地址。
- [std::string_view](cpp17.string-view.md) 可以借用连续字符区间。构造时要提供正确长度，且视图的有效期受原字符数据的生命周期约束。

## 横向对比

| 写法                       | 适用场景                                       |
| -------------------------- | ---------------------------------------------- |
| `std::to_address(p)`       | 统一处理裸指针、指针类对象和连续迭代器         |
| `p.get()`                  | 已知 `p` 是支持 `get()` 的智能指针             |
| `std::addressof(obj)`      | 手头已有活对象的引用，要绕过重载的取地址运算符 |
| `std::ranges::data(range)` | 从支持该操作的范围取得首地址                   |

标准依据见 [P0653R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0653r2.html)。
