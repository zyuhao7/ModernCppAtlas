#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main() {
  std::vector<int> numbers{1, 2, 3};
  std::vector<std::string> words{"a", "bb"};

  auto print = [](const auto& value) { std::cout << value << ' '; };
  std::for_each(numbers.begin(), numbers.end(), print);
  std::for_each(words.begin(), words.end(), print);
  std::cout << '\n';

  auto twice = [](auto value) { return value + value; };
  std::cout << twice(4) << ' ' << twice(std::string("ab")) << '\n';

  auto forward = [](auto&& value) -> decltype(auto) {
    return std::forward<decltype(value)>(value);
  };
  int x = 7;
  int& ref = forward(x);
  ref += 1;
  std::cout << x << ' ' << forward(9) << '\n';
}
