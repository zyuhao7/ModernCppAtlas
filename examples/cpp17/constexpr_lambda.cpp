#include <array>
#include <iostream>

int main() {
  constexpr auto squares = [] {
    std::array<int, 4> values{};
    for (int i = 0; i < 4; ++i) {
      values[i] = i * i;
    }
    return values;
  }();

  static_assert(squares[3] == 9);

  std::cout << "[1] squares =";
  for (int value : squares) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';

  constexpr int scale = 10;
  constexpr auto scaled = [scale](int value) constexpr {
    return value * scale;
  };
  static_assert(scaled(2) == 20);
  std::cout << "[2] scaled(2) = " << scaled(2) << ", scaled(3) = " << scaled(3)
            << '\n';

  constexpr auto make_table = [](int base) {
    std::array<int, 4> values{};
    for (int i = 0; i < 4; ++i) {
      values[i] = base + i;
    }
    return values;
  };
  constexpr auto table = make_table(10);
  static_assert(table[3] == 13);
  std::cout << "[3] table =";
  for (int value : table) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';

  int input = 4;
  std::cout << "[4] runtime scaled(4) = " << scaled(input) << '\n';
}
