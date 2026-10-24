---
id: cpp20.string-range-utilities
title: 字符串与范围小工具
standard: C++20
kind: library
category: [字符串, 容器操作]
difficulty: 2
status: final
tier: A
compilers: {gcc: "10", clang: "13", msvc: "19.24"}
tags: [前缀检查, 删除元素, signed size]
alternatives: [compare, find, remove-erase 惯用法, Boost.StringAlgo]
proposal: P0457R2
last_verified: "2026-10-24"
---

## 解决什么问题

C++20 之前，检查字符串前缀要写 `s.compare(0, prefix.size(), prefix) == 0` 或 `s.find(prefix) == 0`。前者参数顺序容易写错，后者在空串与越界边界上不直观；检查后缀更绕。删除满足条件的元素则要手写 `container.erase(std::remove_if(...), container.end())`，对 `std::string` 和关联容器还要分别处理。

C++20 补了一组小而常用的设施：`starts_with` / `ends_with`、`std::erase` / `std::erase_if`、`std::ssize`。它们不是新容器，也不改变数据结构，只是把高频且易错的写法标准化。`contains` 要分清对象：关联容器的 `contains` 是 C++20；`std::string` 与 `std::string_view` 的 `contains` 到 C++23 才进入标准。

## 基础用法

```cpp
#include <iostream>
#include <string>
#include <vector>

int main() {
  std::string url = "https://example.com/index.html";
  std::cout << url.starts_with("https://") << '\n';
  std::cout << url.ends_with(".html") << '\n';

  std::vector<int> values{1, 2, 3, 4, 5, 6};
  std::erase_if(values, [](int v) { return v % 2 == 0; });

  std::string text = "banana";
  std::erase(text, 'a');
  std::cout << text << ' ' << std::ssize(values) << '\n';
}
```

完整示例见 [`examples/cpp20/string_range_utilities.cpp`](../../examples/cpp20/string_range_utilities.cpp)：

```text
1
1
1
values: 1 3 5
3 bnn
words: apple plum
3
reverse: 5 3 1
```

## 进阶用法

**前缀/后缀检查有三个重载。** `starts_with(char)`、`starts_with(string_view)`、`starts_with(const char*)`。传 `const char*` 时按空终止字符串处理；传 `string_view` 时长度由视图决定，可以检查包含 `'\0'` 的片段。空前缀/后缀恒为真，这与 `find(...) == 0` 的结果一致，但表达更直接。

**`ends_with` 不是反向的 `find`。** `s.rfind(suffix) == s.size() - suffix.size()` 需要处理 `suffix` 比 `s` 长的下溢；`ends_with` 直接处理该边界。

**`std::erase` 与 `std::erase_if` 是自由函数。** `std::erase(container, value)` 删除所有等于 `value` 的元素；`std::erase_if(container, pred)` 删除谓词为真的元素。它们对 `std::vector`、`std::string`、`std::deque` 以及关联容器都有重载。对序列容器保持剩余元素相对顺序，复杂度线性；对关联容器按节点删除。

**不要和成员 `erase` 混淆。** `container.erase(it)` 仍按迭代器删除单个元素；`std::erase(container, value)` 是按值删除全部。成员 `erase` 返回迭代器，自由函数返回删除数量（C++20 起）。

**`std::ssize` 返回有符号大小。** 它把 `size()` 转成 `std::ptrdiff_t`，避免 `size() - 1` 在空容器上回绕成巨大无符号值。对数组、容器和范围都可用；`std::size` 仍返回无符号值。循环里用 `for (auto i = 0; i < std::ssize(v); ++i)` 可以安全地做递减。

**`contains` 的标准版本要分清。** 关联容器的 `contains` 是 C++20；`std::string` 与 `std::string_view` 的 `contains` 都是 C++23。写 C++20 代码时，字符串包含检查仍需 `s.find(part) != npos`。`std::map::contains` 与字符串无关，只是同名。

**异构查找不在这里。** `contains` 不接受透明比较器；`std::map<std::string, V, std::less<>>` 的 `find(string_view)` 是另一项能力，别把两者混为一谈。

## 真实项目证据

| 项目        | 位置                                                                                                                                  | 用法                                           | 为什么典型                                                  |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------- | ----------------------------------------------------------- |
| rui314/mold | [`lib/common.h#L957-L960`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/common.h#L957-L960)       | 检查路径是否以 `/` 开头                        | 把前缀判断写成单个谓词，替代 `compare` 或 `find` 的位置运算 |
| rui314/mold | [`lib/multi-glob.cc#L80-L93`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/multi-glob.cc#L80-L93) | 同时用 `starts_with` 与 `ends_with` 处理通配符 | 前后缀检查成对出现，边界条件由标准库承担                    |
| rui314/mold | [`lib/demangle.cc#L13-L27`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/demangle.cc#L13-L27)     | 在 `std::string_view` 上检查 Itanium 前缀      | 视图上直接判断，不构造 `std::string`                        |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。mold 的构建配置声明 `cxx_std_20`，这些调用发生在生产路径。

## 陷阱与边界

- **`starts_with("")` 恒为真。** 空前缀不是错误；如果业务要求非空前缀，在调用方校验。
- **`const char*` 重载依赖空终止。** 传入的不是以 `'\0'` 结尾的缓冲区会越界读取；二进制数据用 `string_view`。
- **`std::erase` 删除全部匹配项。** 只想删第一个时用 `find` + 成员 `erase`。
- **谓词不要修改容器。** `erase_if` 遍历期间修改容器会导致未定义行为；需要联动删除时先收集再处理。
- **`std::ssize` 不改变底层类型。** 它只是返回有符号值；与无符号 `size()` 混用仍可能触发隐式转换警告。
- **`contains` 的标准版本不同。** C++20 的 `std::string` 与 `std::string_view` 都没有 `contains`；跨标准写作时先确认目标标准。
- **`string_view` 版本不保证空终止。** 用 `starts_with` 后若把视图交给 C API，仍需自行补 `'\0'`。
- **`erase_if` 对 `std::map` 的谓词接收 `value_type`。** 想按 key 删除时，谓词里访问 `.first`，不要误以为参数是 key。

## 库联动

- **mold**：链接器在参数解析与 glob 匹配的热路径上直接使用这些谓词，避免临时字符串与手写索引运算。
- **fmt / spdlog**：格式化库内部有各自的 `string_view` 兼容层；把用户输入转成 `std::string_view` 后再做前缀判断，可以同时适配两套实现。代价是视图生命周期必须覆盖调用。
- **nlohmann/json**：JSON Pointer 有自己的 `contains` 语义，和容器/字符串的 `contains` 不同名同义；混用时以类型为准，不要按名字推断行为。
- **与 ranges 的分工**：`erase_if` 处理容器原地删除；`views::filter` 只做惰性视图，不修改底层数据。需要保留原容器时用视图，需要真正移除时用 `erase_if`。

## 横向对比

| 需求       | C++20 写法               | 旧写法                                  | 主要差异                                      |
| ---------- | ------------------------ | --------------------------------------- | --------------------------------------------- |
| 前缀检查   | `s.starts_with(p)`       | `s.compare(0, p.size(), p) == 0`        | 参数顺序固定，空串与超长边界由库处理          |
| 后缀检查   | `s.ends_with(p)`         | `s.rfind(p) == s.size() - p.size()`     | 避免长度下溢                                  |
| 按值删除   | `std::erase(c, v)`       | `c.erase(std::remove(...), c.end())`    | 一步完成，返回删除数量                        |
| 按条件删除 | `std::erase_if(c, pred)` | `c.erase(std::remove_if(...), c.end())` | 对关联容器也有统一接口                        |
| 有符号大小 | `std::ssize(c)`          | `static_cast<std::ptrdiff_t>(c.size())` | 避免无符号回绕                                |
| 子串包含   | C++23 `s.contains(p)`    | `s.find(p) != npos`                     | C++20 的 `string` 与 `string_view` 均无此成员 |

跨标准演进：C++20 引入 `starts_with`、`ends_with`、`std::erase`、`std::erase_if`、`std::ssize`；C++23 为 `std::string` 与 `std::string_view` 补上 `contains`。C++26 未改变这些接口的语义。
