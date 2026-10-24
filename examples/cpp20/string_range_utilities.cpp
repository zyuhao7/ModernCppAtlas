#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <string_view>
#include <vector>

int main() {
  const std::string url = "https://example.com/index.html";
  std::cout << url.starts_with("https://") << '\n';
  std::cout << url.ends_with(".html") << '\n';
  std::cout << std::string_view(url).starts_with("https") << '\n';

  std::vector<int> values{1, 2, 3, 4, 5, 6};
  std::erase_if(values, [](int value) { return value % 2 == 0; });
  std::cout << "values:";
  for (const int value : values) std::cout << ' ' << value;
  std::cout << '\n';

  std::string text = "banana";
  std::cout << std::erase(text, 'a') << ' ' << text << '\n';

  std::map<std::string, int> word_count{
      {"apple", 2},
      {"pear", 0},
      {"plum", 3},
  };
  std::erase_if(word_count,
                [](const auto& item) { return item.second == 0; });
  std::cout << "words:";
  for (const auto& item : word_count) std::cout << ' ' << item.first;
  std::cout << '\n';

  std::cout << std::ssize(values) << '\n';

  std::cout << "reverse:";
  for (auto i = std::ssize(values) - 1; i >= 0; --i) {
    std::cout << ' ' << values[static_cast<std::size_t>(i)];
  }
  std::cout << '\n';
}
