#include <iostream>
#include <iterator>
#include <regex>
#include <string>

int main() {
  // 场景 1：regex_match 要求整体匹配，捕获组按下标读取。
  std::regex version(R"(^v(\d+)\.(\d+)\.(\d+)$)");
  std::smatch match;
  std::string input = "v1.2.3";
  if (std::regex_match(input, match, version)) {
    std::cout << "[1] groups = " << match.size() - 1
              << ", version = " << match[1] << '.' << match[2] << '.'
              << match[3] << '\n';
  }

  // 场景 2：整体匹配失败时，regex_search 仍能在字符串中找到子串。
  std::regex loose(R"(v(\d+)\.(\d+)\.(\d+))");
  std::string decorated = "release v2.0.1 now";
  std::smatch found;
  const bool whole = std::regex_match(decorated, version);
  const bool partial = std::regex_search(decorated, found, loose);
  std::cout << "[2] match = " << whole << ", search = " << partial
            << ", found = " << found[1] << '.' << found[2] << '.' << found[3]
            << '\n';

  // 场景 3：sregex_iterator 把每个匹配逐个产出。
  std::regex word(R"(\b\w+\b)");
  std::string text = "modern cpp atlas";
  auto count =
      std::distance(std::sregex_iterator(text.begin(), text.end(), word),
                    std::sregex_iterator());
  std::cout << "[3] words = " << count << '\n';

  // 场景 4：regex_replace 按模式替换，这里把连续空白折叠成连字符。
  std::regex spaces(R"(\s+)");
  std::cout << "[4] joined = " << std::regex_replace(text, spaces, "-") << '\n';

  // 场景 5：icase 让匹配忽略大小写。
  std::regex cpp(R"(cpp)", std::regex::icase);
  std::string mixed = "CPP cpp Cpp";
  auto hits =
      std::distance(std::sregex_iterator(mixed.begin(), mixed.end(), cpp),
                    std::sregex_iterator());
  std::cout << "[5] icase hits = " << hits << '\n';

  // 场景 6：替换串可以引用捕获组，交换两个单词的顺序。
  std::regex pair(R"((\w+) (\w+))");
  std::cout << "[6] swapped = "
            << std::regex_replace("modern atlas", pair, "$2 $1") << '\n';
}
