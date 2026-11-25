#include <bit>
#include <cstdint>
#include <iostream>

int main() {
  const std::uint32_t eight = 8;
  const std::uint32_t nine = 9;
  std::cout << "[1] has_single_bit(8) = " << std::has_single_bit(eight)
            << ", has_single_bit(9) = " << std::has_single_bit(nine) << '\n';

  std::cout << "[2] bit_ceil(9) = " << std::bit_ceil(nine)
            << ", bit_floor(9) = " << std::bit_floor(nine) << '\n';

  std::cout << "[3] bit_width(8) = " << std::bit_width(eight)
            << ", bit_width(9) = " << std::bit_width(nine) << '\n';

  const std::uint32_t mask = 0b101100u;
  std::cout << "[4] lowest power-of-2 >= 44 = " << std::bit_ceil(mask)
            << ", highest <= 44 = " << std::bit_floor(mask) << '\n';

  static_assert(std::has_single_bit(64u));
  static_assert(std::bit_ceil(100u) == 128u);
  constexpr std::uint32_t capacity = std::bit_ceil(5u);
  std::cout << "[5] table capacity for 5 slots = " << capacity << '\n';

  const std::uint32_t zero = 0;
  std::cout << "[6] bit_ceil(0) = " << std::bit_ceil(zero)
            << ", bit_floor(0) = " << std::bit_floor(zero)
            << ", bit_width(0) = " << std::bit_width(zero) << '\n';
}
