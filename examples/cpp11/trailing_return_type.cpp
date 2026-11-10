#include <iostream>

template <typename T, typename U>
auto add(T a, U b) -> decltype(a + b) {
  return a + b;
}

int main() {
  std::cout << add(2, 3) << '\n';
  std::cout << add(1.5, 2.0) << '\n';
}
