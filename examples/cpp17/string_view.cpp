#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

std::string_view trim(std::string_view text) {
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) {
    return {};
  }
  const auto last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

std::string_view file_extension(std::string_view path) {
  const auto slash = path.find_last_of('/');
  const auto dot = path.find_last_of('.');
  if (dot == std::string_view::npos ||
      (slash != std::string_view::npos && dot < slash)) {
    return {};
  }
  return path.substr(dot + 1);
}

std::vector<std::string_view> split(std::string_view text, char delimiter) {
  std::vector<std::string_view> parts;
  std::size_t start = 0;
  while (true) {
    const auto pos = text.find(delimiter, start);
    parts.push_back(text.substr(start, pos == std::string_view::npos
                                           ? std::string_view::npos
                                           : pos - start));
    if (pos == std::string_view::npos) {
      break;
    }
    start = pos + 1;
  }
  return parts;
}

int main() {
  // 视图只指向原字符串，trim 与 substr 都不复制字符。
  const std::string line = "  host = example.com  ";
  const auto equals = line.find('=');
  const std::string_view key = trim(std::string_view(line).substr(0, equals));
  const std::string_view value =
      trim(std::string_view(line).substr(equals + 1));
  std::cout << "[1] key = " << key << ", value = " << value << '\n';

  std::cout << "[2] main.cpp -> " << file_extension("src/main.cpp")
            << ", README -> "
            << (file_extension("README").empty() ? "none"
                                                 : file_extension("README"))
            << '\n';

  const std::string csv = "alpha,beta,,gamma";
  const std::vector<std::string_view> parts = split(csv, ',');
  std::cout << "[3] parts = " << parts.size() << ", joined = ";
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i != 0) {
      std::cout << '|';
    }
    std::cout << parts[i];
  }
  std::cout << '\n';

  // 长度由 string_view 携带，C API 只看遇到 '\0' 为止。
  const std::string raw("ab\0cd", 5);
  const std::string_view view(raw);
  std::cout << "[4] view size = " << view.size()
            << ", strlen = " << std::strlen(view.data()) << '\n';

  std::string_view word = "  atlas  ";
  word.remove_prefix(2);
  word.remove_suffix(2);
  std::cout << "[5] stripped = " << word << ", size = " << word.size() << '\n';
}
