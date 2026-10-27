#include <array>
#include <iostream>

constexpr int square(int x) { return x * x; }

constexpr int sum_squares(int n) {
  return n == 0 ? 0 : square(n) + sum_squares(n - 1);
}

constexpr int kSize = sum_squares(3);

int main() {
  static_assert(square(4) == 16, "computed at compile time");
  std::array<int, kSize> table{};
  std::cout << kSize << ' ' << table.size() << '\n';
}
