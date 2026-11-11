#include <concepts>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

template <typename T>
concept Sized = requires(const T& value) {
  { value.size() } -> std::convertible_to<std::size_t>;
};

int main() {
  auto sum = []<typename T>(T lhs, T rhs) { return lhs + rhs; };
  auto count = []<typename... Ts>(Ts...) { return sizeof...(Ts); };
  auto first_size = []<Sized T>(const T& value) { return value.size(); };

  std::cout << "[1] sum(3, 4) = " << sum(3, 4) << '\n';

  std::cout << "[2] count = " << count(1, 2.0, 'x') << '\n';

  std::cout << "[3] sizes = " << first_size(std::vector<int>{1, 2, 3}) << ' '
            << first_size(std::string("atlas")) << '\n';

  auto make = []<typename T>(T value) { return value; };
  auto explicit_value = make.operator()<int>(42);
  std::cout << "[4] explicit = " << explicit_value << '\n';

  auto total = []<typename... Ts>(Ts... values) { return (values + ...); };
  std::cout << "[5] total = " << total(1, 2, 3) << '\n';
}
