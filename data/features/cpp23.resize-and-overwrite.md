---
id: cpp23.resize-and-overwrite
title: std::string::resize_and_overwrite
standard: C++23
kind: library
category: [字符串, 性能]
difficulty: 2
status: final
tier: A
compilers: {gcc: "12", clang: "14", msvc: "19.31"}
tags: [resize_and_overwrite, string, 缓冲区, 零初始化, 字符串拼接]
alternatives: [resize 后写入, reserve 加 append, 手工管理 char 缓冲, std::vector<char>]
proposal: P1072R10
last_verified: "2026-11-18"
claims:
  - id: c1
    statement: "percent 解码按输入长度一次性 resize_and_overwrite，回调直接写入并返回实际长度"
    evidence: [ev-nghttp2-resize-overwrite-percent-decode-001]
  - id: c2
    statement: "字符串拼接前按总长度申请缓冲，回调填写后返回写入长度"
    evidence: [ev-abseil-resize-overwrite-strcat-002]
  - id: c3
    statement: "按能力检测缓冲区是否支持 resize_and_overwrite，支持时用于序列化预留"
    evidence: [ev-glaze-resize-overwrite-buffer-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

向 `std::string` 写入大量已知长度的数据时，常见写法是先 `resize(n)` 把长度撑开，再逐字符覆盖。`resize` 会先把新字符全部值初始化为 `'\0'`，紧接着又会被真实数据覆盖，这部分写入是纯开销。用 `reserve` 加 `append` 可以避免清零，但需要自己维护写入位置，还要在最后同步长度。

C++23 的 `resize_and_overwrite(count, op)` 把“撑开缓冲、写入、设置长度”合成一次调用：库保证 `[p, p + count)` 可写（实际可用空间可能更大），把指针和可用长度交给回调 `op`，回调返回实际写入的字符数，字符串长度随即设为该值，并在末尾补 `'\0'`。中间没有零初始化，也不需要二次遍历。

## 基础用法

回调签名是 `size_type(char* p, size_type n)`。`p` 指向连续缓冲区，`n` 是至少 `count` 的可用长度；返回小于等于 `n` 的值即可，字符串长度取返回值。

<!-- compile -->

```cpp
#include <iostream>
#include <string>

int main() {
  std::string text;
  text.resize_and_overwrite(5, [](char* p, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      p[i] = static_cast<char>('a' + i);
    }
    return n;
  });
  std::cout << text << '\n';
}
```

输出 `abcde`。完整版本见 [`examples/cpp23/resize_and_overwrite.cpp`](../../examples/cpp23/resize_and_overwrite.cpp)，六个场景的输出为：

```text
[1] abcde
[2] 12345
[3] ok size=2
[4] ababab
[5] empty size = 0
[6] feature = 202110
```

场景 1 直接填写 5 个字母。场景 2 用 `std::snprintf` 写入整数，并把返回值当作实际长度。场景 3 申请 8 个字符的空间却只返回 2，字符串收缩为 `ok`。场景 4 交替填写 `a`、`b`。场景 5 用长度 0 清空内容。场景 6 输出版本宏。

注意回调里的 `n` 是缓冲区容量而非请求长度，循环应以 `n` 为上界，但返回的长度不能超过 `n`；返回 0 等价于把字符串清空。

## 进阶用法

**解码时按输入长度预留。** nghttp2 的 percent 解码把输入区间长度作为 `count`，回调直接往缓冲写解码结果，再用 `std::ranges::distance` 算出行进距离作为返回值。输出不会超过输入长度，一次分配即可覆盖最坏情况，省去了逐段 `push_back` 的扩容。

**拼接前算好总长。** abseil 的 `StrCat` 先累加各段长度，再调用 `absl::StringResizeAndOverwrite` 一次性写入。包装层在 C++23 上直接转调标准函数，旧标准上用自己的缓冲实现；上层代码只看到一个入口。

**按能力检测再使用。** glaze 的 `resize_unfilled` 用 `requires` 检测缓冲区类型是否提供 `resize_and_overwrite`：标准 `std::string` 支持就直接预留未初始化空间，不支持的缓冲类型退回 `resize`。这种写法让序列化代码同时适配多种缓冲后端。

**返回值可以小于申请长度。** 预留空间按上界估计、实际结果更短时，返回真实长度即可收缩，不必再调用 `resize` 截断。

**它是 `constexpr` 的。** C++23 起该成员函数可在常量求值中使用，适合在编译期构造固定字符串；回调同样要满足常量求值规则。

## 真实项目证据

| 项目               | 位置                                                                                                                                                                               | 用法                                                       | 为什么典型                                                |
| ------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------- | --------------------------------------------------------- |
| nghttp2/nghttp2    | [`src/util.h#L281-L291`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/util.h#L281-L291)                                                    | percent 解码按输入长度预留，回调写入后返回实际长度         | 输出长度可预估，避免零初始化与逐段扩容，是典型收益场景    |
| abseil/abseil-cpp  | [`absl/strings/str_cat.cc#L88-L95`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/strings/str_cat.cc#L88-L95)                            | `StrCat` 先算总长，再经兼容包装一次性填写缓冲              | 大型基础库为 C++23 提供统一入口，说明它适合高频字符串拼接 |
| stephenberry/glaze | [`include/glaze/core/buffer_traits.hpp#L61-L70`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/core/buffer_traits.hpp#L61-L70) | 用 `requires` 探测缓冲类型是否支持，支持时预留未初始化空间 | 序列化写入需要未初始化内存，能力检测保证跨缓冲后端可移植  |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径，覆盖 URL 解码、字符串拼接与序列化缓冲三种场景。

## 陷阱与边界

**回调必须返回不超过缓冲长度的值。** 返回 `n` 或更小的值才合法；返回更大的值属于未定义行为。把外部函数的返回值直接透传时，要先检查它是否小于 `n`。

**回调内没有终止符。** `p` 指向的区间未初始化，不保证以 `'\0'` 结尾；写入后由库在 `p[返回值]` 处补终止符，不要依赖回调内的字符串函数。

**不要保留指针。** 回调返回后缓冲可能因后续操作失效，`p` 只在回调执行期间有效，不能存下来跨调用使用。

**异常会让字符串状态未指定。** 回调抛异常时异常向外传播，字符串处于有效但未指定的状态；需要保留原内容时，先写入临时字符串再交换。

**`count` 过大抛 `length_error`。** 超过 `max_size()` 的请求会抛异常，而不是返回失败；按外部输入估算长度时要先做上界检查。

**可用空间可能大于请求。** 回调的 `n` 是实际可写容量，可能大于 `count`；按 `n` 循环会把多余位置也写满，但字符串长度只由返回值决定。

**支持时间。** GCC 12、Clang 14、MSVC 19.31 起提供；特性测试宏为 `__cpp_lib_string_resize_and_overwrite >= 202110L`。

## 库联动

- **`std::string`**：该成员函数定义在 `std::basic_string` 上，`std::string` 是最常用实例。
- **`std::string_view`**：拼接前常用视图计算总长，再一次性写入目标缓冲。
- **`std::format`**：格式化结果长度可预估，适合先预留再写入，避免中间临时字符串。
- **`std::vector<char>`**：另一种可变缓冲；不支持该成员函数，需要自己的预留写法或能力检测。
- **`std::ranges::distance`**：nghttp2 用它计算回调写入了多少字符，把迭代器行进距离直接作为长度返回。

## 横向对比

| 方案                   | 零初始化开销 | 长度由谁维护  | 异常安全             | 主要限制                         |
| ---------------------- | ------------ | ------------- | -------------------- | -------------------------------- |
| `resize_and_overwrite` | 无           | 回调返回值    | 回调抛出后状态未指定 | C++23 起；回调内不能用字符串函数 |
| `resize` 后逐字符写    | 有，整段清零 | 调用方先定死  | 强保证               | 多一次全段写入，长度固定         |
| `reserve` + `append`   | 无           | 每次 `append` | 强保证               | 多次调用，需要维护写入位置       |
| 手工 `char` 缓冲       | 无           | 调用方        | 取决于实现           | 要自己管理分配、长度与终止符     |

`resize_and_overwrite` 由 P1072R10 进入 C++23，把“预留、写入、定长”收进一个成员函数。它不改变 `std::string` 的存储模型，也不适合边写边查内容的场景，但在输出长度可预估、需要避免零初始化的路径上，是最直接的写法。
