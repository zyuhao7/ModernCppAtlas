#include <cstddef>
#include <iostream>
#include <vector>

template <typename Container>
auto middle(const Container& values) -> decltype(values.begin()) {
  return values.begin() + static_cast<std::ptrdiff_t>(values.size() / 2);
}

int main() {
  std::vector<int> values{10, 20, 30, 40, 50};
  auto it = middle(values);
  decltype(values.size()) count = values.size();
  std::cout << *it << ' ' << count << '\n';
}
