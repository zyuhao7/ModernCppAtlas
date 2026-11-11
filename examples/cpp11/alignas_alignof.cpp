#include <cstdint>
#include <iostream>

// 场景 1：类型级对齐，每个 Vec4 对象都按 16 字节对齐。
struct alignas(16) Vec4 {
  float values[4];
};

// 场景 2：对齐会放大对象尺寸，大小必须是 16 的倍数。
struct alignas(16) Padded {
  char tag;
};

// 场景 3：按缓存行对齐，隔离并发热点。
struct alignas(64) CacheLine {
  std::uint64_t counter;
};

// 场景 5：alignof 是编译期常量，可以进入模板与 static_assert。
template <typename T>
struct align_report {
  static constexpr std::size_t value = alignof(T);
};

int main() {
  std::cout << "[1] alignof(Vec4) = " << alignof(Vec4)
            << ", sizeof(Vec4) = " << sizeof(Vec4) << '\n';

  std::cout << "[2] alignof(Padded) = " << alignof(Padded)
            << ", sizeof(Padded) = " << sizeof(Padded) << '\n';

  CacheLine slots[2] = {};
  const auto first = reinterpret_cast<std::uintptr_t>(&slots[0]);
  const auto second = reinterpret_cast<std::uintptr_t>(&slots[1]);
  std::cout << "[3] alignof(CacheLine) = " << alignof(CacheLine)
            << ", stride = " << (second - first)
            << ", both aligned = " << (first % 64 == 0 && second % 64 == 0)
            << '\n';

  // 场景 4：多条 alignas 取最严格者，弱的 8 字节要求被 64 覆盖。
  alignas(8) alignas(64) int buffer[4] = {1, 2, 3, 4};
  std::cout << "[4] buffer % 64 = "
            << (reinterpret_cast<std::uintptr_t>(buffer) % 64)
            << ", buffer[3] = " << buffer[3] << '\n';

  // 场景 5
  static_assert(alignof(CacheLine) == 64, "cache line alignment");
  std::cout << "[5] align_report<CacheLine>::value = "
            << align_report<CacheLine>::value << ", static_assert passed = 1"
            << '\n';

  // 场景 6：局部变量也能过对齐。
  alignas(32) int counter = 7;
  std::cout << "[6] counter = " << counter << ", counter % 32 = "
            << (reinterpret_cast<std::uintptr_t>(&counter) % 32) << '\n';
}
