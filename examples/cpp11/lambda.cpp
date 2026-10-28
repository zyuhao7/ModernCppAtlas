#include <algorithm>
#include <iostream>
#include <vector>

int main() {
  std::vector<int> values{3, 1, 4, 1, 5};
  std::sort(values.begin(), values.end(), [](int a, int b) { return a > b; });

  int threshold = 2;
  auto above = [threshold](int value) { return value > threshold; };

  long total = 0;
  std::for_each(values.begin(), values.end(),
                [&total](int value) { total += value; });

  const auto count = std::count_if(values.begin(), values.end(), above);
  std::cout << count << ' ' << total << '\n';
}
