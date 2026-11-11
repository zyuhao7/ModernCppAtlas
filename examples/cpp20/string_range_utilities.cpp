#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <string_view>
#include <vector>

int main() {
  const std::string url = "https://example.com/index.html";
  std::cout << "[1] starts = " << url.starts_with("https://")
            << ", ends = " << url.ends_with(".html")
            << ", view starts = " << std::string_view(url).starts_with("https")
            << '\n';

  std::vector<int> values{1, 2, 3, 4, 5, 6};
  std::erase_if(values, [](int value) { return value % 2 == 0; });
  std::cout << "[2] values =";
  for (const int value : values) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';

  std::string text = "banana";
  const auto removed = std::erase(text, 'a');
  std::cout << "[3] removed = " << removed << ", text = " << text << '\n';

  std::map<std::string, int> word_count{
      {"apple", 2},
      {"pear", 0},
      {"plum", 3},
  };
  std::erase_if(word_count, [](const auto& item) { return item.second == 0; });
  std::cout << "[4] words =";
  for (const auto& item : word_count) {
    std::cout << ' ' << item.first;
  }
  std::cout << '\n';

  std::cout << "[5] size = " << std::ssize(values) << ", reverse =";
  for (auto i = std::ssize(values) - 1; i >= 0; --i) {
    std::cout << ' ' << values[static_cast<std::size_t>(i)];
  }
  std::cout << '\n';

  std::cout << "[6] empty prefix = " << url.starts_with("")
            << ", long suffix = " << url.ends_with(".html.backup") << '\n';
}
