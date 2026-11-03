#include <algorithm>
#include <array>
#include <iostream>
#include <tuple>

int main() {
  std::array<int, 5> values{{3, 1, 4, 1, 5}};

  int sum = 0;
  for (int value : values) sum += value;
  std::cout << values.size() << ' ' << sum << ' ' << values.front() << ' '
            << values.back() << '\n';

  std::cout << std::get<0>(values) << ' '
            << std::tuple_size<decltype(values)>::value << '\n';

  std::array<int, 5> copy = values;
  std::array<int, 5> larger{{3, 1, 4, 1, 6}};
  std::cout << (copy == values ? "equal" : "different") << ' '
            << (copy < larger ? "less" : "greater") << '\n';

  std::sort(values.begin(), values.end());
  std::cout << values[0] << ' ' << values[2] << ' ' << values[4] << '\n';

  values.fill(7);
  std::cout << values.at(0) << ' ' << values.at(4) << '\n';
}
