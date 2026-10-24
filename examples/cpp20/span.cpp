#include <array>
#include <iostream>
#include <span>
#include <vector>

int sum(std::span<const int> values) {
  int total = 0;
  for (int value : values) total += value;
  return total;
}

int main() {
  int raw[] = {1, 2, 3};
  std::array<int, 2> arr{4, 5};
  std::vector<int> vec{6, 7};

  std::cout << sum(raw) << '\n';
  std::cout << sum(arr) << '\n';
  std::cout << sum(vec) << '\n';
}
