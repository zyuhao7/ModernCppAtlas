#include <bit>
#include <cstdint>
#include <iostream>

constexpr std::uint32_t kOneBits = std::bit_cast<std::uint32_t>(1.0f);
static_assert(kOneBits == 0x3f800000u);

int main() {
  std::uint32_t raw = 0x40490fdb;
  float value = std::bit_cast<float>(raw);
  std::uint32_t sign = raw >> 31;
  std::uint32_t exponent = (raw >> 23) & 0xff;
  std::uint32_t mantissa = raw & 0x7fffff;
  std::cout << value << ' ' << sign << ' ' << exponent << ' ' << mantissa
            << '\n';
}
