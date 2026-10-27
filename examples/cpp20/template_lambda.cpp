#include <concepts>
#include <cstddef>
#include <iostream>
#include <vector>

template <typename T>
concept Sized = requires(const T &value) {
  { value.size() } -> std::convertible_to<std::size_t>;
};

int main() {
  auto sum = []<typename T>(T lhs, T rhs) { return lhs + rhs; };
  auto count = []<typename... Ts>(Ts...) { return sizeof...(Ts); };
  auto first_size = []<Sized T>(const T &value) { return value.size(); };

  std::cout << sum(3, 4) << ' ' << count(1, 2.0, 'x') << ' '
            << first_size(std::vector<int>{1, 2, 3}) << '\n';
}
