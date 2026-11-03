#include <iostream>
#include <string>
#include <unordered_map>

int main() {
  std::unordered_map<std::string, int> counts;
  counts["apple"] = 2;
  counts["pear"] += 1;

  auto result = counts.emplace("apple", 5);
  std::cout << counts.size() << ' ' << counts.at("apple") << ' '
            << (result.second ? "new" : "exists") << '\n';
  std::cout << (counts.find("pear") != counts.end() ? "found" : "missing")
            << '\n';
}
