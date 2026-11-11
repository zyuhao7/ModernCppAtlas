#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <vector>

int sum(std::span<const int> values) {
  int total = 0;
  for (int value : values) {
    total += value;
  }
  return total;
}

template <std::size_t N>
int fixed_sum(std::span<const int, N> values) {
  int total = 0;
  for (int value : values) {
    total += value;
  }
  return total;
}

void scale(std::span<int> values, int factor) {
  for (int& value : values) {
    value *= factor;
  }
}

void print(std::span<const int> values) {
  bool first = true;
  for (int value : values) {
    if (!first) {
      std::cout << ' ';
    }
    first = false;
    std::cout << value;
  }
  std::cout << '\n';
}

int main() {
  int raw[] = {1, 2, 3};
  std::array<int, 2> arr{4, 5};
  std::vector<int> vec{6, 7};

  std::cout << "[1] sums = " << sum(raw) << ' ' << sum(arr) << ' ' << sum(vec)
            << '\n';

  std::cout << "[2] fixed sum = " << fixed_sum(std::span<const int, 3>{raw})
            << '\n';

  std::cout << "[3] subspan = ";
  print(std::span<const int>(raw).subspan(1, 2));
  std::cout << "[3] first = ";
  print(std::span<const int>(arr).first(1));

  scale(vec, 10);
  std::cout << "[4] scaled = ";
  print(vec);

  std::cout << "[5] bytes = " << std::as_bytes(std::span(raw)).size() << '\n';

  std::span<const int> empty;
  std::cout << "[6] empty size = " << empty.size() << '\n';
}
