#include <iostream>
#include <map>
#include <string>
#include <vector>

int main() {
  std::vector<std::string> words{"atlas", "modern", "cpp"};

  bool first = true;
  for (const auto& word : words) {
    if (!first) std::cout << ' ';
    first = false;
    std::cout << word;
  }
  std::cout << '\n';

  for (auto& word : words) word += '!';
  std::cout << words.front() << ' ' << words.back() << '\n';

  std::map<std::string, int> counts{{"a", 1}, {"b", 2}};
  int total = 0;
  for (const auto& entry : counts) total += entry.second;
  std::cout << total << ' ' << counts.size() << '\n';

  int raw[] = {3, 1, 4};
  for (int value : raw) total += value;
  std::cout << total << '\n';
}
