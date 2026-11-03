#include <array>
#include <iostream>

int main() {
  std::array<int, 5> values{{3, 1, 4, 1, 5}};

  int sum = 0;
  for (int value : values) sum += value;
  std::cout << values.size() << ' ' << sum << ' ' << values.front() << ' '
            << values.back() << '\n';

  values.fill(7);
  std::cout << values[0] << ' ' << values.at(4) << '\n';
}
