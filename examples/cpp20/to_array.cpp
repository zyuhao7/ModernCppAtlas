#include <array>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

enum class Color { Red, Green, Blue };

int main() {
  constexpr auto digits = std::to_array({1, 2, 3, 4, 5});
  std::cout << "[1] size = " << digits.size() << ", front = " << digits.front()
            << ", back = " << digits.back() << '\n';

  constexpr auto names =
      std::to_array<std::string_view>({"red", "green", "blue"});
  std::cout << "[2] names = " << names[0] << ' ' << names[2] << '\n';

  constexpr auto labels = std::to_array<std::pair<Color, std::string_view>>(
      {{Color::Red, "red"}, {Color::Green, "green"}});
  std::cout << "[3] first = " << labels[0].second
            << ", last = " << labels[1].second << '\n';

  std::cout << "[4] is std::array = "
            << std::is_same<decltype(digits), const std::array<int, 5>>::value
            << '\n';

  auto mutable_values = std::to_array<double>({1.5, 2.5});
  mutable_values[0] = 4.5;
  std::cout << "[5] updated = " << mutable_values[0] << '\n';

  const auto copy = digits;
  std::cout << "[6] copy size = " << copy.size()
            << ", sum = " << copy[0] + copy[4] << '\n';
}
