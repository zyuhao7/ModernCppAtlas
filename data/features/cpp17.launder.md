---
id: cpp17.launder
title: std::launder
standard: C++17
kind: library
category: [对象模型, 内存管理]
difficulty: 4
status: final
tier: A
compilers: {gcc: "7", clang: "6", msvc: "19.14"}
tags: [launder, 生命周期, placement new, 优化, 指针]
alternatives: [placement new 的返回值, std::construct_at, reinterpret_cast, 直接使用存储地址]
proposal: P0137R1
last_verified: "2026-11-13"
claims:
  - id: c1
    statement: "手动生命周期容器在裸缓冲区上就地构造后用 std::launder 取得可用指针并返回引用"
    evidence: [ev-stdexec-launder-manual-lifetime-001]
  - id: c2
    statement: "惰性单例从字节存储取对象指针前用 std::launder 重新获取"
    evidence: [ev-abseil-launder-no-destructor-002]
  - id: c3
    statement: "constexpr 存储辅助类用 std::launder 把字节数组转成元素指针"
    evidence: [ev-glaze-launder-byte-storage-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

在原始存储上就地构造对象是容器、`optional`、单例和手动生命周期管理的常规手段，但语言有一条容易被忽视的规则：一个 `T*` 一旦指向某对象，它携带了关于该对象的“来源”信息。当同一块存储上结束旧对象、再构造一个新对象时，旧指针未必能用于访问新对象，即使地址完全相同。

`std::launder` 解决的是这个指针来源（provenance）问题。它接收一个指向已知对象的指针，返回一个可以合法访问该对象的指针；地址不变，但返回的指针去掉了对旧对象的来源绑定。P0137R1 引入它的同时收紧了指针互转的规则，`launder` 成为在再构造之后“重新取得”合法指针的标准手段。它的典型用途是就地构造后取回指针、以及访问含 `const` 或引用成员的对象。

## 基础用法

`std::launder(p)` 返回一个 `T*`，地址与 `p` 相同，但可用于访问 `p` 所指向位置上的活跃对象。头文件是 `<new>`。

<!-- compile -->

```cpp
#include <cstddef>
#include <iostream>
#include <new>

struct Point {
  int x;
  int y;
};

int main() {
  alignas(Point) std::byte storage[sizeof(Point)];
  new (storage) Point{3, 4};
  Point* p = std::launder(reinterpret_cast<Point*>(storage));
  std::cout << p->x << ' ' << p->y << '\n';
  std::destroy_at(p);
}
```

输出：

```text
3 4
```

完整版本见 [`examples/cpp17/launder.cpp`](../../examples/cpp17/launder.cpp)，六个场景的输出为：

```text
[1] id = 7, payload = 100
[2] same address = 1
[3] payload = 200
[4] id = 9, payload = 300
[5] squares = 0 1 4
[6] destroyed all objects
```

场景 1 在栈上对齐存储里构造含 `const` 成员的结构体，用 `launder` 取得指针。场景 2 确认 `launder` 返回的地址与原存储一致。场景 3 通过 laundered 指针修改成员。场景 4 销毁旧对象、在同址构造新对象后再次 `launder`。场景 5 在一段缓冲区里构造数组，用 `launder` 得到首元素指针。场景 6 逐个销毁，配平构造。

## 进阶用法

**再构造之后必须重新 launder。** 旧对象的生命周期结束后，即使地址没变，指向它的指针也不能拿来访问新对象。销毁再构造后要重新走一次 `launder`，这是它最核心的使用场景。

**含 `const` 或引用成员时更需注意。** 若对象类型有 `const` 成员，标准允许在旧对象生命周期结束后、用指向它的指针做有限操作；`launder` 能把这类指针恢复成可正常访问新对象的指针。

**和 placement new 搭配。** 常见写法是 `T* p = std::launder(reinterpret_cast<T*>(buf));` 紧接 `new (buf) T{...};`，或用 placement new 的返回值后再 `launder`。stdexec 的手动生命周期容器就返回 `*std::launder(::new (...) _Ty{...})`，一次构造即拿到可用引用。

**在常量求值里也能用。** `launder` 是 `constexpr` 函数，glaze 的 `byte_storage` 在与 `constexpr` 兼容的路径上用它取得元素指针，让就地容器既能在运行期也能在编译期工作。

**不要滥用。** `launder` 比普通指针互转更受限，不应到处加。它只在“同一存储上换了活跃对象”或“访问含 `const`/引用成员的再构造对象”这类明确场景才需要；无脑包一层不会带来好处，反而掩盖真实意图。

## 真实项目证据

| 项目           | 位置                                                                                                                                                                                               | 用法                                                           | 为什么典型                                              |
| -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------- | ------------------------------------------------------- |
| NVIDIA/stdexec | [`include/stdexec/__detail/__manual_lifetime.hpp#L46-L54`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/__detail/__manual_lifetime.hpp#L46-L54) | placement new 构造后用 `std::launder` 取得可用指针并返回引用   | 就地在裸缓冲区构造，返回引用前必须消除旧对象的来源绑定  |
| abseil-cpp     | [`absl/base/no_destructor.h#L157-L168`](https://github.com/abseil/abseil-cpp/blob/2065f4ded0558c6f89fee67c8e5228feb4eb960e/absl/base/no_destructor.h#L157-L168)                                    | 从字节存储取对象指针前用 `std::launder` 重新获取               | 惰性单例构造一次永不销毁，取用时用 launder 保证指针合法 |
| glaze          | [`include/glaze/containers/inplace_vector.hpp#L37-L44`](https://github.com/stephenberry/glaze/blob/d78832c82289c61a9315bfbc35332cec9f4e93ca/include/glaze/containers/inplace_vector.hpp#L37-L44)   | `constexpr` 存储辅助类用 `std::launder` 把字节数组转成元素指针 | 元素存在对齐的字节数组里，launder 在常量求值下也用得到  |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在生产路径上，覆盖手动生命周期容器、惰性单例与 `constexpr` 就地容器。

## 陷阱与边界

**不是类型转换。** `std::launder` 不改变类型，也不做任何位操作；`reinterpret_cast` 负责把字节地址变成 `T*`，`launder` 负责修正来源。两者职责不同，别互相替代。

**它不构造对象。** `launder` 只返回指针，不会替你 placement new。先有活跃对象，才有可 launder 的目标；对没有对象的地址调用它是未定义行为。

**地址不变不代表指针可互用。** 最容易踩的坑是“反正地址一样就直接复用旧指针”。在一段存储上结束旧对象、构造新对象后，旧指针可能失效，必须重新 `launder`。

**过度使用有害。** 在不需要的场景加 `launder` 会误导读者，以为存在特殊的生命周期问题。只在该用的地方用。

**编译器优化会放大错误。** P0137R1 之后，编译器可基于来源规则做更激进的优化；漏掉 `launder` 的代码可能在小端、低优化级别下“看起来正常”，换到高优化就出问题。

**旧标准没有它。** GCC 7、Clang 6、MSVC 19.14 起才提供。C++17 之前只能依赖编译器宽容，没有标准手段表达这一意图。

## 库联动

- **placement new / `std::construct_at`**：就地构造的好搭档，构造后接 `launder` 取回指针。
- **`std::destroy_at`**：结束对象生命周期的入口，与再构造、再 launder 构成完整循环。
- **`std::bit_cast`**：同样处理对象表示，但做的是类型间搬运；与 `launder` 属于不同维度的工具。
- **`std::optional` / 容器实现**：在裸存储上管理元素生命周期的容器，内部普遍用到 `launder` 及其同类。

## 横向对比

| 方案                   | 修正来源 | 只返回指针 | 主要限制                               |
| ---------------------- | -------- | ---------- | -------------------------------------- |
| `std::launder`         | 是       | 是         | C++17 起；仅限明确场景，不构造对象     |
| placement new 的返回值 | 部分     | 否（构造） | 只覆盖构造那一刻，再构造后仍需 launder |
| `reinterpret_cast<T*>` | 否       | 是         | 不改来源，再构造后可能失效             |
| 直接用存储地址         | 否       | 是         | 违反来源规则，是未定义行为             |

`std::launder` 由 P0137R1 进入 C++17，是那把“在存储上就地构造对象”这一底层能力写进标准规则的配套件。它不产生新地址，也不构造对象，只把来源信息修正到当前活跃对象上，让容器、单例和手动生命周期代码在严格规则下依然可移植。
