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
  std::cout << squares[0] << ' ' << squares[3] << '\n';
}
