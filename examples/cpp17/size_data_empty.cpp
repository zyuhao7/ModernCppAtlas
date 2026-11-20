#include <array>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <vector>

template <class Container>
std::size_t element_count(const Container& container) {
  return std::size(container);
}

int main() {
  int raw[] = {10, 20, 30, 40};
  std::vector<int> values{1, 2, 3};
  std::array<char, 5> letters{'h', 'e', 'l', 'l', 'o'};

  std::cout << "[1] raw size = " << std::size(raw) << '\n';
  std::cout << "[2] vector size = " << std::size(values) << '\n';
  std::cout << "[3] template size = " << element_count(letters) << '\n';
  std::cout << "[4] first = " << *std::data(raw) << '\n';
  std::cout << "[5] vector data = " << *std::data(values) << '\n';

  std::vector<int> none;
  std::cout << "[6] empty raw = " << std::empty(raw)
            << ", empty none = " << std::empty(none) << '\n';
}
