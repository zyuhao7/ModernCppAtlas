#include <algorithm>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

int main() {
  std::unordered_map<std::string, int> counts;
  counts.reserve(8);

  // 场景 1：operator[] 读不存在的键会插入值初始化的元素。
  counts["apple"] = 2;
  counts["pear"] += 1;
  counts["grape"];
  std::cout << "[1] after operator[] size = " << counts.size()
            << ", pear = " << counts.at("pear")
            << ", grape = " << counts.at("grape") << '\n';

  // 场景 2：emplace 在键已存在时不覆盖，返回 bool 说明是否插入。
  auto first = counts.emplace("apple", 5);
  auto second = counts.emplace("banana", 3);
  std::cout << "[2] emplace apple inserted = "
            << (first.second ? "true" : "false")
            << ", emplace banana inserted = "
            << (second.second ? "true" : "false")
            << ", apple = " << counts.at("apple") << '\n';

  // 场景 3：find 区分命中与未命中，count 在无序容器里只可能是 0 或 1。
  std::cout << "[3] find(pear) = "
            << (counts.find("pear") != counts.end() ? "found" : "missing")
            << ", count(kiwi) = " << counts.count("kiwi") << '\n';

  // 场景 4：at 越界抛 out_of_range，operator[] 则会静默插入。
  try {
    counts.at("missing");
  } catch (const std::out_of_range&) {
    std::cout << "[4] at(missing) throws out_of_range, size = " << counts.size()
              << '\n';
  }

  // 场景 5：erase 按键删除，返回实际删除的数量。
  std::size_t erased = counts.erase("pear");
  std::cout << "[5] erase(pear) = " << erased
            << ", count(pear) = " << counts.count("pear") << '\n';

  // 场景 6：遍历顺序取决于哈希，复制到 vector 排序后才能稳定输出。
  std::vector<std::pair<std::string, int>> ordered(counts.begin(),
                                                   counts.end());
  std::sort(ordered.begin(), ordered.end());
  std::cout << "[6]";
  for (const auto& entry : ordered) {
    std::cout << ' ' << entry.first << '=' << entry.second;
  }
  std::cout << '\n';
}
