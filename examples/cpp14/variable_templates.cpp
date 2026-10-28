#include <iostream>

template <typename T>
constexpr T pi = T(3.1415926535897932385);

template <typename T>
constexpr bool is_small = sizeof(T) <= sizeof(int);

int main() {
  static_assert(is_small<char>, "char fits in int");
  static_assert(!is_small<double>, "double is wider than int");

  std::cout << pi<double> << ' ' << pi<float> << '\n';
}
