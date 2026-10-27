---
id: cpp17.pmr
title: 多态内存资源（std::pmr）
standard: C++17
kind: library
category: [内存管理, 容器]
difficulty: 4
status: final
tier: A
compilers: {gcc: "9.1", clang: "16", msvc: "19.13"}
tags: [pmr, memory_resource, 分配器, 内存池, 多态]
alternatives: [std::allocator 与自定义分配器, 内存池, 单调缓冲区, 重载 operator new]
proposal: P0220R1
last_verified: "2026-10-26"
claims:
  - id: c1
    statement: "实现 std::pmr::memory_resource 子类，用 cudaMallocHost 提供固定页内存"
    evidence: [ev-stdexec-pmr-pinned-resource-001]
  - id: c2
    statement: "用同一个 memory_resource 接口封装 cudaMalloc 的设备内存"
    evidence: [ev-stdexec-pmr-gpu-resource-002]
  - id: c3
    statement: "让单调缓冲资源按块向上游 memory_resource 申请内存"
    evidence: [ev-stdexec-pmr-monotonic-upstream-003]
review:
  author: "zyuhao7"
  level: manual
  reviewers: ["zyuhao7"]
---

## 解决什么问题

传统分配器是容器类型的一部分：`std::vector<int, MyAlloc>` 和 `std::vector<int>` 是不同类型。想按运行时条件选择内存来源，只能把分配器类型一路模板化，或者用类型擦除包一层。前者让类型签名膨胀，后者要自己管理生命周期。

`std::pmr` 把分配策略从类型参数移到运行时对象。容器统一使用 `std::pmr::polymorphic_allocator<T>`，其中只保存一个 `std::pmr::memory_resource*`；这个指针指向真正的分配器，可以是默认的 `new`/`delete`、单调缓冲区、内存池或用户自定义资源。同一个 `std::pmr::vector<int>` 类型可以在不同资源上实例化。标准提案为 P0220R1。

## 基础用法

`memory_resource` 的派生类覆盖 `do_allocate`、`do_deallocate` 和 `do_is_equal`，容器通过资源分配对象。

<!-- compile -->

```cpp
#include <iostream>
#include <memory_resource>
#include <vector>

int main() {
  std::pmr::monotonic_buffer_resource pool;
  std::pmr::vector<int> values(&pool);

  for (int i = 0; i < 5; ++i) {
    values.push_back(i * i);
  }

  int sum = 0;
  for (int value : values) {
    sum += value;
  }

  std::cout << values.size() << ' ' << sum << '\n';
}
```

输出 `5 30`。`values` 的所有分配都来自 `pool`，`pool` 析构时统一释放；完整版本见 [`examples/cpp17/pmr.cpp`](../../examples/cpp17/pmr.cpp)。

标准库提供几种现成资源：`new_delete_resource()` 与 `null_memory_resource()`、按块分配且不单独回收的 `monotonic_buffer_resource`、面向重复块大小的 `unsynchronized_pool_resource` 和 `synchronized_pool_resource`。`get_default_resource()` 返回进程默认资源，`std::pmr::vector<T>` 不传资源时就使用它。

## 进阶用法

自定义资源只需实现三个虚函数。`do_allocate` 返回满足对齐的存储，失败时抛 `std::bad_alloc`；`do_deallocate` 只能接收本资源此前分配的内存；`do_is_equal` 定义两个资源何时可以互相释放对方分配的内存。资源相同或 `is_equal` 为真时，内存才能跨容器转移。

`monotonic_buffer_resource` 适合短生命周期、批量释放的场景：可以给它一块初始栈缓冲区，用完后向 `upstream` 资源继续申请块，但不会逐个回收，直到资源析构。解析器、请求处理、编译器前端常把它当作用户态 arena。

嵌套容器会传播资源。`std::pmr::vector<std::pmr::string>` 的元素在构造时通过 `polymorphic_allocator` 的传播规则获得同一资源，不需要为每个元素手工传参。这个机制依赖元素类型支持 `uses_allocator` 构造。

资源相等性影响所有权。两个 `polymorphic_allocator` 比较相等，当且仅当它们持有相同资源或 `do_is_equal` 返回真；容器移动、交换和 `deallocate` 都按这个规则检查。自定义资源若按地址比较，就要保证同一份内存在同一资源对象上释放。

## 真实项目证据

| 项目    | 位置                                                                                                                                                                       | 用法                                                                                   | 为什么典型                                       |
| ------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------- | ------------------------------------------------ |
| stdexec | [`include/nvexec/detail/memory.cuh#L146-L160`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/nvexec/detail/memory.cuh#L146-L160) | `pinned_resource` 继承 `std::pmr::memory_resource`，用 `cudaMallocHost` 分配固定页内存 | 自定义资源把设备 API 接到标准分配接口            |
| stdexec | [`include/nvexec/detail/memory.cuh#L177-L191`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/nvexec/detail/memory.cuh#L177-L191) | `gpu_resource` 用 `cudaMalloc` 分配设备内存                                            | 同一抽象覆盖主机、固定页与设备内存               |
| stdexec | [`include/nvexec/detail/memory.cuh#L252-L260`](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/nvexec/detail/memory.cuh#L252-L260) | `monotonic_buffer_resource` 的构造函数向 `upstream->allocate` 取首块                   | 资源组合：单调分配器只负责块管理，底层来源可替换 |

三条证据均为 `manual` 级：作者复核，尚未引入第二名审核者。三处都在 stdexec 的 CUDA 后端中，说明 pmr 不只用于主机内存，也能把 GPU 分配器接入同一套容器接口。

## 陷阱与边界

**资源必须比容器活得久。** 容器只保存 `memory_resource*`，不拥有资源。函数返回一个使用局部资源的容器，或静态容器引用已销毁的静态资源，都会产生悬空指针。

**释放必须回到等价资源。** `deallocate` 要求资源相同或 `is_equal` 为真。跨资源传递容器内容前先确认相等性，否则析构时可能把内存还给错误的分配器。

**`monotonic_buffer_resource` 不逐个释放。** 每次 `deallocate` 都是空操作，内存只在资源析构时统一归还。长时间运行、反复增长的循环里直接使用会持续占用内存。

**每次都多一次虚调用。** `polymorphic_allocator` 通过虚函数分配和释放，还多存一个指针。分配极其频繁且类型固定的热路径，普通模板分配器可能更合适。

**并非所有容器都有 pmr 别名。** `std::pmr` 为常用顺序、关联、无序容器和 `basic_string` 提供别名，其他容器要显式写 `polymorphic_allocator`。`std::pmr::vector<int>` 与 `std::vector<int>` 仍是不同类型，接口边界上要么模板化，要么转换。

**默认资源是进程级的。** `set_default_resource` 改变的是全局状态，多线程同时设置会产生竞争；已经创建的容器不会自动切换资源。

**跨 ABI 边界要谨慎。** 资源通过虚函数调用分配，动态库两侧必须共享同一套 `memory_resource` 布局和分配约定；跨编译器或跨标准库版本传递 pmr 容器通常不安全。

## 库联动

- **stdexec**：CUDA 后端用 pmr 资源统一主机固定页、设备内存和托管内存，容器与执行器共享同一分配接口。
- **标准容器与字符串**：`std::pmr::vector`、`std::pmr::string`、`std::pmr::unordered_map` 等别名直接接受资源指针。
- **arena 模式**：请求处理、解析和编译器前端用 `monotonic_buffer_resource` 把大量小分配合并成块分配，结束时一次性释放。
- **内存池**：`unsynchronized_pool_resource` 适合单线程重复分配同一尺寸；跨线程共享时换 `synchronized_pool_resource`。
- **自定义设备分配器**：GPU、共享内存或持久化存储都可实现为 `memory_resource` 子类，再交给标准容器使用。

## 横向对比

| 方案                        | 分配器类型进入容器类型 | 运行时切换资源 | 释放粒度   | 主要代价                       |
| --------------------------- | ---------------------- | -------------- | ---------- | ------------------------------ |
| 标准模板分配器              | 是                     | 否             | 逐对象     | 类型签名膨胀，组合困难         |
| `std::pmr`                  | 否                     | 是             | 由资源决定 | 每次分配一次虚调用，多一个指针 |
| `monotonic_buffer_resource` | 否                     | 是             | 整块释放   | 不能逐个回收，内存会持续增长   |
| 重载全局 `operator new`     | 否                     | 全局           | 全局       | 影响整个进程，难以按模块隔离   |

跨标准演进：C++17 通过 P0220R1 引入 `<memory_resource>` 与 pmr 别名；它把分配策略从编译期类型参数转成运行期对象，代价是间接调用和显式的资源生命周期管理。
