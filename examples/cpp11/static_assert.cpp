#include <cstdint>
#include <iostream>
#include <type_traits>

template <typename T>
T checked_square(T value) {
  static_assert(std::is_integral<T>::value,
                "checked_square requires an integral type");
  return value * value;
}

int main() {
  static_assert(sizeof(std::int32_t) == 4, "int32_t must be 4 bytes");
  std::cout << checked_square(7) << '\n';
}
