#include <iostream>
#include <map>
#include <string>
#include <vector>

int main() {
  std::vector<std::string> words{"atlas", "modern", "cpp"};

  // 场景 1：const auto& 只读遍历，不复制元素。
  std::cout << "[1]";
  for (const auto& word : words) std::cout << ' ' << word;
  std::cout << '\n';

  // 场景 2：auto& 是引用，改写会作用到容器元素上。
  for (auto& word : words) word += '!';
  std::cout << "[2] front = " << words.front() << ", back = " << words.back()
            << '\n';

  // 场景 3：map 的元素是 pair<const string, int>，用 const auto& 读取。
  std::map<std::string, int> counts{{"a", 1}, {"b", 2}};
  int total = 0;
  for (const auto& entry : counts) total += entry.second;
  std::cout << "[3] total = " << total << ", entries = " << counts.size()
            << '\n';

  // 场景 4：C 数组也能按范围遍历；auto 按值取副本，改动不回写数组。
  int raw[] = {3, 1, 4};
  int sum = 0;
  for (int value : raw) sum += value;
  for (auto value : raw) value *= 10;
  std::cout << "[4] sum = " << sum << ", raw[0] = " << raw[0] << '\n';
}
