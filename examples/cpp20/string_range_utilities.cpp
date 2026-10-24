#include <iostream>
#include <iterator>
#include <string>
#include <vector>

int main() {
  std::string url = "https://example.com/index.html";
  std::cout << url.starts_with("https://") << '\n';
  std::cout << url.ends_with(".html") << '\n';

  std::vector<int> values{1, 2, 3, 4, 5, 6};
  std::erase_if(values, [](int v) { return v % 2 == 0; });

  std::string text = "banana";
  std::erase(text, 'a');
  std::cout << text << ' ' << std::ssize(values) << '\n';
}
