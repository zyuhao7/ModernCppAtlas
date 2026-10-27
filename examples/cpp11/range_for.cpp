#include <iostream>
#include <map>
#include <string>
#include <vector>

int main() {
  std::vector<std::string> words{"atlas", "modern", "cpp"};
  for (const auto &word : words) std::cout << word << ' ';
  std::cout << '\n';

  std::map<std::string, int> counts{{"a", 1}, {"b", 2}};
  int total = 0;
  for (const auto &entry : counts) total += entry.second;
  std::cout << total << '\n';
}
