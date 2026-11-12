#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>

int main() {
  const std::uint32_t mask = 0b10110100u;
  std::cout << "[1] popcount = " << std::popcount(mask) << '\n';

  const std::uint32_t isolated = 0b00010000u;
  std::cout << "[2] trailing = " << std::countr_zero(isolated)
            << ", leading = " << std::countl_zero(isolated) << '\n';

  const std::uint8_t ones = 0b00000111u;
  std::cout << "[3] trailing ones = " << std::countr_one(ones)
            << ", leading ones = " << std::countl_one(ones) << '\n';

  const std::uint32_t rotated = std::rotl(mask, 4);
  const std::uint32_t restored = std::rotr(rotated, 4);
  std::cout << "[4] rotl = " << rotated
            << ", round trip = " << (restored == mask) << '\n';

  const std::uint64_t alignment = 1ull << 12;
  const std::uint64_t recovered = 1ull << std::countr_zero(alignment);
  std::cout << "[5] alignment = " << alignment
            << ", shift = " << std::countr_zero(alignment)
            << ", recovered = " << recovered << '\n';

  const std::uint32_t value = 1000;
  const int width =
      std::numeric_limits<std::uint32_t>::digits - std::countl_zero(value);
  std::cout << "[6] width = " << width << ", highest = " << width - 1 << '\n';
}
