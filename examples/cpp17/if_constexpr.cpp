#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>

template <typename T>
auto deref_or_value(const T& value) {
  if constexpr (std::is_pointer_v<T>) {
    return *value;
  } else {
    return value;
  }
}

template <typename T>
std::string describe(const T& value) {
  if constexpr (std::is_pointer_v<T>) {
    return describe(*value);
  } else if constexpr (std::is_integral_v<T>) {
    return "integer:" + std::to_string(value);
  } else if constexpr (std::is_floating_point_v<T>) {
    return "floating:" + std::to_string(value);
  } else {
    return "other";
  }
}

template <typename... Ts>
constexpr std::size_t count_integral() {
  return (std::size_t{0} + ... + (std::is_integral_v<Ts> ? 1u : 0u));
}

int main() {
  int number = 42;

  std::cout << deref_or_value(number) << '\n';
  std::cout << deref_or_value(&number) << '\n';

  std::cout << describe(number) << '\n';
  std::cout << describe(3.5) << '\n';
  std::cout << describe(&number) << '\n';

  static_assert(count_integral<int, double, long>() == 2);
}
