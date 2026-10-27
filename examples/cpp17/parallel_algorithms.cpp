#include <algorithm>
#include <execution>
#include <iostream>
#include <numeric>
#include <vector>

int main() {
  std::vector<int> values(10);
  std::iota(values.begin(), values.end(), 1);

  std::sort(std::execution::par, values.begin(), values.end(),
            [](int lhs, int rhs) { return lhs > rhs; });
  const int sum =
      std::reduce(std::execution::par, values.begin(), values.end());

  std::cout << values.front() << ' ' << values.back() << ' ' << sum << '\n';
}
