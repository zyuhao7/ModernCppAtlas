#include <cstdint>
#include <iostream>
#include <limits>
#include <utility>

int main() {
  const int signed_value = -1;
  const unsigned unsigned_value = 1;

  // Usual arithmetic conversions turn -1 into its unsigned bit pattern, so the
  // comparison is not between -1 and 1 as written.
  std::cout << "[1] -1 as unsigned = " << static_cast<unsigned>(signed_value)
            << ", cmp_less(-1, 1u) = "
            << std::cmp_less(signed_value, unsigned_value) << '\n';

  std::cout << "[2] cmp_greater(1u, -1) = "
            << std::cmp_greater(unsigned_value, signed_value) << '\n';

  std::cout << "[3] cmp_equal(1u, 1) = " << std::cmp_equal(unsigned_value, 1)
            << ", cmp_not_equal(1u, -1) = "
            << std::cmp_not_equal(unsigned_value, signed_value) << '\n';

  const std::size_t count = 3;
  const int limit = 5;
  std::cout << "[4] size_t 3 < int 5 = " << std::cmp_less(count, limit) << '\n';

  const std::int64_t big = std::numeric_limits<std::int64_t>::max();
  const std::uint32_t small = std::numeric_limits<std::uint32_t>::max();
  std::cout << "[5] cmp_greater(int64_max, uint32_max) = "
            << std::cmp_greater(big, small) << '\n';

  static_assert(std::cmp_less(-1, 1u));
  static_assert(!std::cmp_equal(-1, std::numeric_limits<unsigned>::max()));
}
