#include <algorithm>
#include <concepts>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

template <typename T>
void append(std::vector<T>& x, const auto& y) {
  x.insert(x.end(), y.begin(), y.end());
}

void sort(auto& vec) { std::sort(vec.begin(), vec.end()); }

void sort(auto& vec, auto less) { std::sort(vec.begin(), vec.end(), less); }

void advance(std::integral auto& counter) { ++counter; }

void swap_values(std::same_as<int> auto& lhs, std::same_as<int> auto& rhs) {
  std::swap(lhs, rhs);
}

void print_range(const auto& range) {
  bool first = true;
  for (const auto& value : range) {
    if (!first) {
      std::cout << ' ';
    }
    std::cout << value;
    first = false;
  }
}

int main() {
  std::vector<int> numbers{3, 1, 2};
  append(numbers, std::vector{4, 0});
  sort(numbers);
  std::cout << "[1] ascending = ";
  print_range(numbers);
  std::cout << '\n';

  sort(numbers, std::greater<>{});
  std::cout << "[2] descending = ";
  print_range(numbers);
  std::cout << '\n';

  std::vector<std::string> words{"a", "b"};
  append(words, std::vector<std::string>{"c"});
  std::cout << "[3] words = ";
  print_range(words);
  std::cout << '\n';

  int counter = 41;
  advance(counter);
  unsigned ticket = 7;
  advance(ticket);
  std::cout << "[4] counter = " << counter << ", ticket = " << ticket << '\n';

  int lhs = 7;
  int rhs = 9;
  swap_values(lhs, rhs);
  std::cout << "[5] swapped = " << lhs << ' ' << rhs << '\n';
}
