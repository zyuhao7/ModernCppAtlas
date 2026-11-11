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
  std::cout << "[1] sorted front = " << values.front()
            << ", back = " << values.back() << '\n';

  std::vector<int> doubled(values.size());
  std::transform(std::execution::par, values.begin(), values.end(),
                 doubled.begin(), [](int value) { return value * 2; });
  std::cout << "[2] doubled front = " << doubled.front()
            << ", back = " << doubled.back() << '\n';

  const int sum =
      std::reduce(std::execution::par, values.begin(), values.end());
  std::cout << "[3] sum = " << sum << '\n';

  const auto even =
      std::count_if(std::execution::par, values.begin(), values.end(),
                    [](int value) { return value % 2 == 0; });
  std::cout << "[4] even count = " << even << '\n';

  const int ordered = std::accumulate(values.begin(), values.end(), 0);
  std::cout << "[5] ordered sum = " << ordered
            << ", policy independent = " << (ordered == sum) << '\n';
}
