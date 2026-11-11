#include <cstdint>
#include <iostream>
#include <type_traits>

template <typename T>
T checked_square(T value) {
  static_assert(std::is_integral<T>::value,
                "checked_square requires an integral type");
  return value * value;
}

// 类内断言锁定二进制布局，与外部协议格式保持一致。
struct PacketHeader {
  std::uint16_t version;
  std::uint16_t length;
  std::uint32_t checksum;
};
static_assert(sizeof(PacketHeader) == 8, "header must stay 8 bytes");

enum class Priority { low = 1, normal = 2, high = 3 };
// 无符号比较依赖取值顺序，断言把这条隐含约定写死。
static_assert(Priority::low < Priority::normal &&
                  Priority::normal < Priority::high,
              "priority ordering must increase");

int main() {
  // 场景 1：平台假设在编译期校验，失败时能直接看到消息。
  static_assert(sizeof(std::int32_t) == 4, "int32_t must be 4 bytes");
  std::cout << "[1] int32_t bytes = " << sizeof(std::int32_t) << '\n';

  // 场景 2：模板内部的断言在实例化时检查；传入浮点会编译失败。
  std::cout << "[2] checked_square(7) = " << checked_square(7) << '\n';

  // 场景 3：类布局断言同样不产生运行期代码。
  std::cout << "[3] header bytes = " << sizeof(PacketHeader) << '\n';

  // 场景 4：不变量断言把枚举顺序变成编译器强制检查。
  std::cout << "[4] priority order ok = " << std::boolalpha
            << (Priority::low < Priority::high) << '\n';
}
