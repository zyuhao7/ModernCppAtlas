#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

int main() {
  std::unordered_map<std::string, int> counts;
  counts.reserve(8);

  counts["apple"] = 2;
  counts["pear"] += 1;
  auto result = counts.emplace("apple", 5);
  counts.insert(std::make_pair(std::string("banana"), 3));

  std::cout << counts.size() << ' ' << counts.at("apple") << ' '
            << (result.second ? "new" : "exists") << '\n';

  std::cout << (counts.find("pear") != counts.end() ? "found" : "missing")
            << ' ' << counts.count("grape") << '\n';

  counts.erase("pear");
  std::cout << counts.size() << ' '
            << (counts.count("pear") ? "kept" : "erased") << '\n';

  try {
    counts.at("missing");
  } catch (const std::out_of_range&) {
    std::cout << "out_of_range" << '\n';
  }

  std::vector<std::pair<std::string, int>> ordered(counts.begin(),
                                                   counts.end());
  std::sort(ordered.begin(), ordered.end());
  for (std::size_t i = 0; i < ordered.size(); ++i) {
    if (i != 0) std::cout << ' ';
    std::cout << ordered[i].first << '=' << ordered[i].second;
  }
  std::cout << '\n';
}
