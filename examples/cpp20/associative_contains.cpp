#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

int main() {
  std::map<std::string, int> scores{{"alice", 90}, {"bob", 75}};
  std::set<int> ids{3, 5, 8};
  std::unordered_map<std::string, int> cache{{"GET /", 200}};
  std::unordered_set<std::string> blocked{"spam"};

  std::cout << "[1] alice = " << scores.contains("alice")
            << ", carol = " << scores.contains("carol") << '\n';
  std::cout << "[2] 5 = " << ids.contains(5) << ", 6 = " << ids.contains(6)
            << '\n';
  std::cout << "[3] cached = " << cache.contains("GET /") << '\n';
  std::cout << "[4] spam = " << blocked.contains("spam") << '\n';

  if (!scores.contains("carol")) scores.emplace("carol", 60);
  std::cout << "[5] size = " << scores.size()
            << ", carol = " << scores.at("carol") << '\n';

  const auto it = scores.find("bob");
  std::cout << "[6] found = " << (it != scores.end()) << '\n';
}
