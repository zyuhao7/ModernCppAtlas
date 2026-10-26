#include <algorithm>
#include <iostream>
#include <vector>

template <typename T>
void append(std::vector<T> &x, const auto &y) {
  x.insert(x.end(), y.begin(), y.end());
}

void sort(auto &vec) { std::sort(vec.begin(), vec.end()); }

void sort(auto &vec, auto less) { std::sort(vec.begin(), vec.end(), less); }

int main() {
  std::vector<int> numbers{3, 1, 2};
  append(numbers, std::vector{4, 0});
  sort(numbers);
  sort(numbers, [](int a, int b) { return a > b; });

  for (int number : numbers) {
    std::cout << number << '\n';
  }
}
