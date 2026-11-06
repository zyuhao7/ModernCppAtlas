---
id: cpp11.regex
title: 正则表达式（std::regex）
standard: C++11
kind: library
category: [字符串, 算法]
difficulty: 3
status: final
tier: A
compilers: {gcc: "4.9", clang: "2.8", msvc: "19.0"}
tags: [std::regex, 正则表达式, 模式匹配, 文本处理]
alternatives: [手写解析器, std::string 查找, RE2, PCRE]
proposal: N1429
last_verified: "2026-11-06"
claims:
  - id: c1
    statement: "用静态 regex 解析连接串并逐段提取键值"
    evidence: [ev-drogon-regex-conn-string-001]
  - id: c2
    statement: "静态编译 glob 正则并用 regex_match 判定整个模式"
    evidence: [ev-mold-regex-simple-glob-002]
  - id: c3
    statement: "用捕获组解析内核版本号"
    evidence: [ev-mold-regex-kernel-version-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

从字符串里按模式提取字段时，手写扫描器要处理转义、分隔符和错误分支，代码容易变长；而 `std::string::find` 只能匹配固定子串，无法表达“数字后跟可选单位”这类结构。

`std::regex` 把模式编译成自动机，提供匹配、搜索、替换和迭代四类操作。它适合配置解析、日志提取、命令行校验等中等复杂度的文本匹配。标准提案为 N1429，头文件是 `<regex>`。

## 基础用法

`std::regex` 构造时编译模式，默认使用 ECMAScript 语法。`regex_match` 要求整个输入匹配，`regex_search` 只要求找到一段，捕获组通过 `std::smatch` 按下标读取。

<!-- compile -->

```cpp
#include <iostream>
#include <iterator>
#include <regex>
#include <string>

int main() {
  std::regex version(R"(^v(\d+)\.(\d+)\.(\d+)$)");
  std::smatch match;
  std::string input = "v1.2.3";
  if (std::regex_match(input, match, version)) {
    std::cout << match.size() - 1 << ' ' << match[1] << '.' << match[2] << '.'
              << match[3] << '\n';
  }

  std::regex word(R"(\b\w+\b)");
  std::string text = "modern cpp atlas";
  auto count = std::distance(std::sregex_iterator(text.begin(), text.end(), word),
                             std::sregex_iterator());
  std::cout << count << '\n';

  std::regex spaces(R"(\s+)");
  std::cout << std::regex_replace(text, spaces, "-") << '\n';

  std::smatch found;
  if (std::regex_search(text, found, std::regex(R"((\w+) (\w+))"))) {
    std::cout << found[0] << '|' << found[2] << '\n';
  }
}
```

输出 `3 1.2.3`、`3`、`modern-cpp-atlas` 与 `modern cpp|cpp`。`match.size() - 1` 是捕获组数量（不含整体匹配），`sregex_iterator` 逐个产出单词，`regex_replace` 把连续空白折叠成连字符。完整版本见 [`examples/cpp11/regex.cpp`](../../examples/cpp11/regex.cpp)。

## 进阶用法

**复用编译结果。** 正则编译成本明显高于匹配，循环或高频接口应把 `std::regex` 放到 `static` 或长期对象里；C++11 的静态局部变量初始化是线程安全的。

**选择正确的入口。** `regex_match` 校验完整输入，适合版本号、标识符；`regex_search` 在任意位置查找；`sregex_iterator` 遍历全部匹配；`regex_replace` 按格式串重写。

**显式指定语法与标志。** 默认语法是 ECMAScript，也可选 `basic`、`extended`、`awk`、`grep`、`egrep`。`std::regex_constants::optimize` 只是实现提示，不改变语义；`icase` 忽略大小写。

**捕获组按下标访问。** 下标 0 是整体匹配，1 起是括号组。标准库只保证按位置读取；命名捕获的语法支持因实现而异，跨编译器项目应优先用下标。

**匹配结果引用输入。** `smatch` 保存的是输入序列的迭代器/指针，输入字符串销毁后不能再读 `match[0]`；需要长期保存时复制成 `std::string`。

## 真实项目证据

| 项目   | 位置                                                                                                                                                                 | 用法                             | 为什么典型                                         |
| ------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------- | -------------------------------------------------- |
| drogon | [`orm_lib/src/DbConnection.cc#L21-L30`](https://github.com/drogonframework/drogon/blob/3aae769cc405cf5abb9d477675c6430924f02c7c/orm_lib/src/DbConnection.cc#L21-L30) | 静态 regex 解析数据库连接串      | 模式固定、反复调用，静态编译后循环复用同一份自动机 |
| mold   | [`lib/multi-glob.cc#L75-L78`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/lib/multi-glob.cc#L75-L78)                                | `regex_match` 判断简单 glob 模式 | 需要整体匹配而不是子串搜索，入口选择直接对应语义   |
| mold   | [`src/cmdline.cc#L546-L555`](https://github.com/rui314/mold/blob/27162691b71a3bc84ac4c9b4b9d7c214fad317d3/src/cmdline.cc#L546-L555)                                  | 捕获组解析内核版本号             | 正则同时完成格式校验和字段提取，失败时回退到全零   |

三条证据均为 `manual` 级：作者复核过出处与行号，尚未引入第二名审核者。三处都在生产路径，覆盖连接串、glob 判定与版本解析三类文本任务。

## 陷阱与边界

**回溯可能爆炸。** ECMAScript 语法允许嵌套量词，恶意或粗心的模式可能产生指数级回溯；处理不可信输入时要限制长度，或改用 RE2 这类线性时间引擎。

**不是所有语法都跨实现一致。** `std::regex` 的语法由标准规定，但命名捕获、字符类细节和错误信息在 libstdc++、libc++ 与 MSVC STL 之间仍有差异；跨平台代码应使用共同子集并写测试。

**编译失败抛异常。** 非法模式在构造 `std::regex` 时抛 `std::regex_error`；如果模式来自配置或用户输入，需要在边界处捕获并给出可读错误。

**性能通常不如专用引擎。** 标准库实现为了通用性保留较多抽象，简单前缀/后缀判断用 `find`、`starts_with` 更快；正则适合模式本身复杂、调用次数可控的场景。

**按字节而非 Unicode 匹配。** `std::regex` 对 `char` 序列工作，`\w`、`\d` 等类按当前 locale 解释，不保证按 UTF-8 码点切分；多语言文本要明确编码边界。

**静态对象的初始化时机。** 函数内 `static const std::regex` 在首次调用时编译，可能把异常推迟到第一次请求；启动阶段就应触发的校验，最好在初始化路径显式构造。

## 库联动

- **`<regex>`**：`std::regex`、`std::smatch`、`std::sregex_iterator` 与 `regex_replace` 构成完整接口。
- **`<string>`**：输入与替换结果通常是 `std::string`；`string_view` 不能直接作为匹配目标，需要迭代器区间或转换。
- **`<algorithm>` / `<iterator>`**：正则迭代器可以与标准算法组合，但多数场景直接用 `sregex_iterator` 更清晰。
- **RE2 / Boost.Regex**：需要线性时间保证、更多语法或更好性能时，常用这些替代实现。
- **drogon / mold**：分别展示连接串解析、glob 判定与版本提取三种生产用法。

## 横向对比

| 方案               | 表达力     | 最坏时间复杂度 | 编译成本 | 主要代价                       |
| ------------------ | ---------- | -------------- | -------- | ------------------------------ |
| `std::regex`       | 中高       | 可能指数回溯   | 构造时   | 实现差异、性能不稳定           |
| 手写解析器         | 取决于实现 | 可控           | 无       | 代码量大，边界条件容易漏       |
| RE2                | 高         | 线性           | 构造时   | 第三方依赖，部分高级语法不支持 |
| `std::string` 查找 | 低         | 线性           | 无       | 只能匹配固定子串，无法表达结构 |

跨标准演进：C++11 通过 N1429 引入 `<regex>`；后续标准没有改变核心接口，主要修正细节并扩大实现一致性。C++17 的 `std::string_view` 改善了输入传递方式，但正则算法本身仍按字符序列工作。
