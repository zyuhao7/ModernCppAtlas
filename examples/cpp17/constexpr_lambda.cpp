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

  constexpr int scale = 10;
  constexpr auto scaled = [scale](int value) constexpr {
    return value * scale;
  };
  static_assert(scaled(2) == 20);

  std::cout << squares[0] << ' ' << squares[3] << ' ' << scaled(3) << '\n';
}
