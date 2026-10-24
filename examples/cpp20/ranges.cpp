#include <algorithm>
#include <iostream>
#include <ranges>
#include <vector>

int main() {
  std::vector<int> values{5, 1, 4, 2, 3};
  std::ranges::sort(values);

  for (int value : values | std::views::filter([](int v) { return v % 2 == 1; }) |
                            std::views::transform([](int v) { return v * 10; })) {
    std::cout << value << ' ';
  }
  std::cout << '\n';
}
