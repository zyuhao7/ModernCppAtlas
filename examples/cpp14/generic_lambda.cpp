#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

int main() {
  std::vector<int> numbers{1, 2, 3};
  std::vector<std::string> words{"a", "bb"};

  auto print = [](const auto &value) { std::cout << value << ' '; };
  std::for_each(numbers.begin(), numbers.end(), print);
  std::for_each(words.begin(), words.end(), print);
  std::cout << '\n';
}
