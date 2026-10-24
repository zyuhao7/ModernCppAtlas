#include <iostream>
#include <string>
#include <string_view>

std::string_view file_extension(std::string_view path) {
  auto slash = path.find_last_of('/');
  auto dot = path.find_last_of('.');
  if (dot == std::string_view::npos ||
      (slash != std::string_view::npos && dot < slash)) {
    return {};
  }
  return path.substr(dot + 1);
}

int main() {
  std::string path = "src/main.cpp";
  std::cout << file_extension(path) << '\n';
  std::cout << file_extension("README") << '\n';
}
