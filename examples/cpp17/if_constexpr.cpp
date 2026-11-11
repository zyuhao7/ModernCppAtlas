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

struct Opaque {};

int main() {
  int number = 42;

  std::cout << "[1] value = " << deref_or_value(number)
            << ", pointer = " << deref_or_value(&number) << '\n';

  std::cout << "[2] int = " << describe(number) << '\n';
  std::cout << "[2] double = " << describe(3.5) << '\n';
  std::cout << "[2] pointer = " << describe(&number) << '\n';

  static_assert(count_integral<int, double, long>() == 2);
  std::cout << "[3] integral count = " << count_integral<int, double, long>()
            << '\n';

  std::cout
      << "[4] opaque passthrough = "
      << std::is_same_v<decltype(deref_or_value(Opaque{})), Opaque> << '\n';
}
