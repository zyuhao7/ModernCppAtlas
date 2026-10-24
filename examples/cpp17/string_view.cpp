#include <iostream>
#include <string>
#include <string_view>
#include <vector>

std::string_view trim(std::string_view text) {
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) return {};
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
    if (pos == std::string_view::npos) break;
    start = pos + 1;
  }
  return parts;
}

int main() {
  const std::string line = "  host = example.com  ";
  const auto equals = line.find('=');

  const std::string_view key = trim(std::string_view(line).substr(0, equals));
  const std::string_view value =
      trim(std::string_view(line).substr(equals + 1));
  std::cout << '[' << key << "] [" << value << "]\n";

  std::cout << file_extension("src/main.cpp") << '\n';
  std::cout << file_extension("README") << '\n';

  const std::string csv = "alpha,beta,,gamma";
  for (const auto part : split(csv, ',')) {
    std::cout << '[' << part << ']';
  }
  std::cout << '\n';
}
