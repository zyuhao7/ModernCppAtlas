#include <iostream>
#include <iterator>
#include <regex>
#include <string>

int main() {
  std::regex version(R"(^v(\d+)\.(\d+)\.(\d+)$)");
  std::smatch match;
  std::string input = "v1.2.3";
  if (std::regex_match(input, match, version)) {
    std::cout << match.size() - 1 << ' ' << match[1] << '.' << match[2] << '.'
              << match[3] << '\n';
  }

  std::regex word(R"(\b\w+\b)");
  std::string text = "modern cpp atlas";
  auto count =
      std::distance(std::sregex_iterator(text.begin(), text.end(), word),
                    std::sregex_iterator());
  std::cout << count << '\n';

  std::regex spaces(R"(\s+)");
  std::cout << std::regex_replace(text, spaces, "-") << '\n';

  std::smatch found;
  if (std::regex_search(text, found, std::regex(R"((\w+) (\w+))"))) {
    std::cout << found[0] << '|' << found[2] << '\n';
  }
}
