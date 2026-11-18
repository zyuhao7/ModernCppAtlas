#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

int main() {
  std::vector<int> numbers{1, 2, 3, 4, 5, 6};
  const auto removed_even =
      std::erase_if(numbers, [](int value) { return value % 2 == 0; });
  std::cout << "[1] removed = " << removed_even << ", left =";
  for (int value : numbers) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';

  std::map<std::string, int> scores{{"a", 10}, {"b", 50}, {"c", 90}};
  const auto removed_low = std::erase_if(
      scores, [](const auto& entry) { return entry.second < 50; });
  std::cout << "[2] removed = " << removed_low << ", size = " << scores.size()
            << '\n';

  std::unordered_map<int, std::string> names{
      {1, "one"}, {2, "two"}, {3, "three"}};
  std::erase_if(names, [](const auto& entry) { return entry.first > 1; });
  std::cout << "[3] size = " << names.size() << ", kept = " << names.at(1)
            << '\n';

  std::vector<int> values{1, 2, 2, 3, 2};
  const auto erased = std::erase(values, 2);
  std::cout << "[4] erased = " << erased << ", size = " << values.size()
            << '\n';

  std::vector<int> odds{1, 3, 5};
  const auto removed_none =
      std::erase_if(odds, [](int value) { return value % 2 == 0; });
  std::cout << "[5] removed = " << removed_none << '\n';
}
