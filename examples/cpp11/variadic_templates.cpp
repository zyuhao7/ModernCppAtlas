#include <cstddef>
#include <iostream>
#include <string>

std::size_t count_args() { return 0; }

template <typename T, typename... Rest>
std::size_t count_args(const T &, const Rest &...rest) {
  return 1 + count_args(rest...);
}

void print_all() { std::cout << '\n'; }

template <typename T, typename... Rest>
void print_all(const T &first, const Rest &...rest) {
  std::cout << first << ' ';
  print_all(rest...);
}

int main() {
  print_all(1, "two", 3.5);
  std::cout << count_args(1, "two", 3.5) << '\n';
}
