---
id: cpp17.filesystem
title: std::filesystem
standard: C++17
kind: library
category: [文件系统, 路径处理]
difficulty: 3
status: final
tier: A
compilers: { gcc: "8", clang: "7", msvc: "19.14" }
tags: [路径, 目录遍历, 文件属性]
alternatives: [POSIX 系统调用, Win32 API, Boost.Filesystem]
proposal: P0218R1
last_verified: "2026-10-24"
---

## 解决什么问题

C++17 之前，跨平台文件操作要分叉：POSIX 用 `stat`、`opendir`、`unlink`，Windows 用 `CreateFile`、`FindFirstFile`。路径拼接靠手写分隔符，编码在 `char` 与 `wchar_t` 之间来回转换，错误处理混用 `errno` 和 `GetLastError`。Boost.Filesystem 能解决，但引入整库依赖。

`std::filesystem` 把路径表示、属性查询、目录遍历和文件操作收进标准库，并提供统一的 `error_code` / 异常两条错误通道。它是**库设施**，不改变语言规则；核心价值是消除平台分支与路径字符串拼装错误。

## 基础用法

```cpp
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main() {
  fs::path p = "/var/log/app/server.log";
  std::cout << p.parent_path().string() << '\n';
  std::cout << p.stem().string() << '\n';
  std::cout << p.extension().string() << '\n';
  std::cout << (p.parent_path() / "backup.log").generic_string() << '\n';
}
```

路径分解是纯词法操作，不访问磁盘；后半段用临时目录演示创建、遍历与属性查询。完整示例见 [`examples/cpp17/filesystem.cpp`](../../examples/cpp17/filesystem.cpp)：

```text
/var/log/app
server .log
/var/log/app/backup.log
entries: archive config.ini server.log
size: 5
relative: logs/server.log
```

## 进阶用法

**词法操作与文件系统操作要分开理解。** `parent_path`、`filename`、`lexically_normal` 只处理字符串；`exists`、`canonical`、`file_size` 会触碰磁盘。`canonical` 要求路径存在，`weakly_canonical` 允许末尾不存在，适合“准备创建新文件”的场景。

**优先使用 `error_code` 重载。** 每个会失败的函数都有 `(..., std::error_code&)` 版本。批量扫描目录时，异常版本会在单个权限错误处中断；`error_code` 版本可以记录后继续：

```cpp
std::error_code ec;
for (const auto& entry : fs::directory_iterator(dir, ec)) {
  if (ec) break;
  // ...
}
```

注意：范围 `for` 里的 `directory_iterator` 在自增时仍可能抛异常；要完全无异常，需要手动调用 `it.increment(ec)`。

**区分 `status` 与 `symlink_status`。** `status` 跟随符号链接，`symlink_status` 返回链接自身。删除链接用 `remove` 只删链接；遍历时想跳过链接目录，需先判断 `is_symlink`。

**路径字符串的编码随平台变化。** `string()` 使用原生窄字符表示，Windows 上可能抛 `filesystem_error`；`generic_string()` 统一用 `/`；`u8string()` 在 C++17 返回 `std::string`，C++20 起返回 `std::u8string`，这是源码级不兼容点。需要稳定 UTF-8 时显式用 `generic_u8string()` 并固定标准版本。

**目录遍历有递归与非递归两类。** `recursive_directory_iterator` 支持 `disable_recursion_pending()` 剪枝、`skip_permission_denied` 跳过无权限目录。遍历顺序未指定，不要依赖出现顺序。

**文件大小与时间不是原子快照。** `file_size`、`last_write_time` 是两次独立查询，文件可能在查询之间被修改。需要一致性时自己打开文件并读取元数据。

## 真实项目证据

| 项目 | 位置 | 用法 | 为什么典型 |
|---|---|---|---|
| fmtlib/fmt | [`include/fmt/std.h#L102-L111`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/std.h#L102-L111) | 从 `filesystem::path` 取出指定字符类型的原生字符串 | 把 `path` 当作可查询对象，按目标字符类型选择转换路径 |
| fmtlib/fmt | [`include/fmt/std.h#L305-L313`](https://github.com/fmtlib/fmt/blob/1be298e1bd68957e4cd352e1f676f00e07dcfb57/include/fmt/std.h#L305-L313) | 为 `std::filesystem::path` 提供格式化器 | 库边界上适配标准类型，而不是要求用户先转 `string` |
| nlohmann/json | [`include/nlohmann/detail/meta/std_fs.hpp#L21-L28`](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/include/nlohmann/detail/meta/std_fs.hpp#L21-L28) | 用命名空间别名统一 `std::filesystem` 与实验版 | 兼容旧标准库时集中隔离差异，业务代码只认 `std_fs` |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。

## 陷阱与边界

- **`canonical` 会失败于不存在路径。** 构造目标路径时用 `weakly_canonical`，或先 `absolute` 再 `lexically_normal`。
- **`exists` 之后再打开存在 TOCTOU 窗口。** 文件可能在两次调用之间被删除或替换；安全检查不能只靠 `exists`。
- **`remove` 不递归。** 非空目录会失败；删目录树用 `remove_all`，并注意它会删除符号链接本身而非目标。
- **遍历时删除元素会破坏迭代器。** 需要删除时先收集路径，遍历结束后统一处理。
- **路径比较是词法的。** `path("a/../b") != path("b")`；大小写敏感性由文件系统决定，标准库不做规范化。
- **隐式转换不存在。** `path` 不会自动转成 `std::string`；需要显式 `.string()` 或 `.generic_string()`。
- **流输出会给路径加引号。** `std::cout << p` 按引用形式输出（例如 `"/var/log/app"`），与 `.string()` 的输出不同；日志格式需要稳定文本时显式转换。
- **`u8string()` 的返回类型在 C++20 变了。** 跨标准编译的代码不要写 `auto s = p.u8string();` 再按 `std::string` 使用。
- **权限位不等于访问控制。** `permissions()` 映射到 POSIX 位，Windows ACL 无法完整表达；不要把它当作安全边界。

## 库联动

- **fmt**：`fmt/std.h` 直接为 `std::filesystem::path` 提供格式化器，支持 `{}`、`{:g}`（generic 分隔符）和调试转义。好处是日志里不再手动 `.string()`；代价是引入 `fmt/std.h` 后所有 `path` 格式化都走该实现，Windows 宽字符路径会触发 UTF-8 转换。
- **nlohmann/json**：通过 `detail::std_fs` 别名兼容 `std::filesystem` 与 `std::experimental::filesystem`，读写路径字段时不必关心底层标准库版本。
- **spdlog**：日志消息常直接拼接路径；结合 `fmt` 的 `path` 格式化器可避免临时 `std::string`，但路径中含非 UTF-8 字节时仍要自行处理编码。

## 横向对比

| 方案 | 可移植性 | 错误模型 | 主要代价 |
|---|---|---|---|
| `std::filesystem` | 标准库覆盖的平台 | 异常或 `error_code` | 实现质量随标准库版本变化；路径编码差异仍在 |
| POSIX API | 类 Unix | `errno` | Windows 需要另一套实现 |
| Win32 API | Windows | `GetLastError` | 不可移植，宽字符路径更自然 |
| Boost.Filesystem | 广泛 | 异常或 `error_code` | 额外依赖；与标准版行为有细微差异 |
| C `FILE*` / `fopen` | 广泛 | 返回值 + `errno` | 只处理文件内容，不覆盖目录与属性 |

跨标准演进：C++17 引入 `std::filesystem`；C++20 改变 `u8string()` 的返回类型；后续标准未再改动核心接口。C++26 的文件系统扩展仍以库提案形式讨论，使用前应确认目标标准库实现。
