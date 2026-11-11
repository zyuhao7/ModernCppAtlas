#include <bit>
#include <cstdint>
#include <iostream>
#include <type_traits>

constexpr std::uint32_t kOneBits = std::bit_cast<std::uint32_t>(1.0f);
static_assert(kOneBits == 0x3f800000u);

int main() {
  std::cout << std::hex << std::uppercase;
  std::cout << "[1] bits of 1.0f = " << kOneBits << '\n';

  const std::uint32_t raw = 0x40490fdb;
  const float value = std::bit_cast<float>(raw);
  std::cout << std::dec << "[2] value = " << value << ", sign = " << (raw >> 31)
            << ", exponent = " << ((raw >> 23) & 0xff)
            << ", mantissa = " << (raw & 0x7fffff) << '\n';

  const std::uint32_t back = std::bit_cast<std::uint32_t>(value);
  std::cout << std::hex << std::uppercase
            << "[3] round trip = " << (back == raw) << ", bits = " << back
            << '\n';

  const std::int32_t negative = -1;
  const std::uint32_t bits = std::bit_cast<std::uint32_t>(negative);
  const std::int32_t restored = std::bit_cast<std::int32_t>(bits);
  std::cout << "[4] unsigned = " << bits << ", signed = " << std::dec
            << restored << '\n';

  std::cout << "[5] same size = " << (sizeof(float) == sizeof(std::uint32_t))
            << ", trivially copyable = "
            << std::is_trivially_copyable_v<float> << '\n';
}
