#include <iostream>
#include <numeric>
#include <utility>

namespace {

std::pair<int, int> reduce(int num, int den) {
  const int divisor = std::gcd(num, den);
  return {num / divisor, den / divisor};
}

}  // namespace

int main() {
  std::cout << "[1] gcd(48, 18) = " << std::gcd(48, 18) << '\n';
  std::cout << "[2] lcm(21, 6) = " << std::lcm(21, 6) << '\n';

  const auto [num, den] = reduce(42, 56);
  std::cout << "[3] reduce 42/56 = " << num << '/' << den << '\n';

  std::cout << "[4] coprime(35, 64) = " << (std::gcd(35, 64) == 1) << '\n';
  std::cout << "[5] gcd(0, 9) = " << std::gcd(0, 9) << '\n';

  static_assert(std::gcd(12, 18) == 6);
  static_assert(std::lcm(4, 6) == 12);
}
