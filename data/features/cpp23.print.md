---
id: cpp23.print
title: std::print / std::println
standard: C++23
kind: library
category: [格式化输出, I/O]
difficulty: 2
status: final
tier: B
compilers: {gcc: "14", clang: "18", msvc: "19.37"}
tags: [格式化, 类型安全, 标准输出]
alternatives: [std::cout, printf, std::format, fmt::print]
proposal: P2093R14
last_verified: "2026-10-29"
---

## 解决什么问题

`std::cout` 的输出需要串联 `<<`，格式控制靠 manipulator，写日志和诊断信息时样板代码很多。`printf` 的格式串和参数类型不匹配时，编译器只能给出有限警告，运行时才可能出错。`std::format` 解决了类型安全问题，但要先把结果构造成 `std::string`，再写入输出流。

`std::print` 和 `std::println` 把格式化与输出合并成一次调用：格式串使用 `{}` 占位符，参数类型在编译期检查；结果直接写到标准输出或指定的 `FILE*`。`println` 在结尾追加换行，`print` 不追加。

## 基础用法

<!-- compile -->

```cpp
#include <version>

#if defined(__cpp_lib_print)
#include <print>
#else
#include <iostream>
#endif

int main() {
#if defined(__cpp_lib_print)
  std::println("hello, {}", "world");
  std::println("{} + {} = {}", 2, 3, 2 + 3);
#else
  std::cout << "hello, world!\n";
  std::cout << 2 << " + " << 3 << " = " << 2 + 3 << '\n';
#endif
}
```

输出：

```text
hello, world!
2 + 3 = 5
```

`std::println(stderr, "...", args...)` 可以直接写标准错误；`std::print` 的 `FILE*` 重载适合已经有文件句柄的输出路径。格式串是字符串字面量时，占位符数量、参数类型和格式说明会在编译期检查；运行期拼接的格式串需要走运行期格式化接口。

上面的示例带特性检测：GCC 13 的 libstdc++ 还没有 `<print>`，会用 `std::cout` 分支保持可编译；GCC 14、Clang 18 及更新版本会走 `std::println` 分支。

## 真实项目证据

| 项目            | 位置                                                                                                                                            | 用法                                                      | 为什么典型                                                  |
| --------------- | ----------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------- | ----------------------------------------------------------- |
| nghttp2/nghttp2 | [`src/app_helper.cc#L149-L153`](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/src/app_helper.cc#L149-L153)   | 把 HTTP/2 名称值对格式化输出到 `FILE*`，配合 ANSI 转义    | 使用 `println` 的文件重载，避免先构造字符串；格式串集中可读 |
| hyprwm/Hyprland | [`hyprctl/src/main.cpp#L56-L61`](https://github.com/hyprwm/Hyprland/blob/0aa6745f705f2c4b0a5004eb019ef76e5bbdb426/hyprctl/src/main.cpp#L56-L61) | 日志包装函数在安静模式下直接返回，否则调用 `std::println` | 把 `println` 当作轻量日志出口，调用方不再手工拼接字符串     |

两条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **格式串必须是编译期常量。** 字符串字面量会做编译期检查；把格式串放进变量后，普通 `std::print` 重载无法直接接受，需要显式使用运行期格式化接口。不要把用户输入当作格式串。
- **花括号要转义。** 输出字面量 `{` 或 `}` 时写成 `{{` 和 `}}`，否则会被当成占位符。
- **`print` 不追加换行。** 需要换行时用 `println`，或在格式串末尾显式写 `\n`；不要把 `println` 当成自动 flush。
- **不要和 `std::cout` 混用。** 两套输出通道的缓冲不同，混用时顺序可能不符合预期；同一段输出路径统一用一种方式。
- **`FILE*` 重载不接管句柄生命周期。** 传入的 `FILE*` 必须保持有效；写失败的处理方式由调用方决定，不要假设 `println` 会重试。
- **自定义类型需要 `std::formatter`。** `std::print` 复用 `std::format` 的格式化规则；没有特化 `std::formatter` 的类型不能直接输出。
- **编译器支持晚于语言标准。** GCC 13 和旧版 libstdc++ 没有 `<print>`；GCC 14、Clang 18（libc++/libstdc++ 满足要求时）、MSVC 19.37 起可用。

## 横向对比

| 方案                     | 类型安全 | 编译期检查格式串 | 直接输出         | 主要代价                              |
| ------------------------ | -------- | ---------------- | ---------------- | ------------------------------------- |
| `std::cout`              | 是       | 否               | 是               | 格式控制冗长，混用 `printf` 时易出错  |
| `printf`                 | 否       | 有限             | 是               | 格式串与参数不匹配时是未定义行为      |
| `std::format` + 输出     | 是       | 是               | 否，先构造字符串 | 多一次字符串分配和拷贝                |
| `fmt::print`             | 是       | 是               | 是               | 需要引入外部依赖，但支持 C++20 及以前 |
| `std::print` / `println` | 是       | 是               | 是               | 需要 C++23 库支持；格式串必须是字面量 |

`std::print` 从 C++23 起可用；C++20 及以前的项目通常用 `fmt::print` 过渡，迁移时注意格式串写法和自定义类型的 `formatter` 特化。
