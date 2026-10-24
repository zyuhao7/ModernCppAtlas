#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <vector>

int sum(std::span<const int> values) {
  int total = 0;
  for (int value : values) total += value;
  return total;
}

template <std::size_t N>
int fixed_sum(std::span<const int, N> values) {
  int total = 0;
  for (int value : values) total += value;
  return total;
}

void scale(std::span<int> values, int factor) {
  for (int& value : values) value *= factor;
}

void print(std::span<const int> values) {
  bool first = true;
  for (int value : values) {
    if (!first) std::cout << ' ';
    first = false;
    std::cout << value;
  }
  std::cout << '\n';
}

int main() {
  int raw[] = {1, 2, 3};
  std::array<int, 2> arr{4, 5};
  std::vector<int> vec{6, 7};

  std::cout << sum(raw) << '\n';
  std::cout << sum(arr) << '\n';
  std::cout << sum(vec) << '\n';

  std::cout << fixed_sum(std::span<const int, 3>{raw}) << '\n';

  print(std::span<const int>(raw).subspan(1, 2));
  print(std::span<const int>(arr).first(1));

  scale(vec, 10);
  print(vec);

  std::cout << std::as_bytes(std::span(raw)).size() << '\n';
}
