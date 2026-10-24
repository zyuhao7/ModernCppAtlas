#include <charconv>
#include <iostream>
#include <optional>
#include <string_view>

std::optional<int> parse_int(std::string_view text) {
  int value = 0;
  auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || ptr != text.data() + text.size()) {
    return std::nullopt;
  }
  return value;
}

int main() {
  if (auto parsed = parse_int("42")) {
    std::cout << *parsed << '\n';
  }
  std::cout << parse_int("nope").value_or(-1) << '\n';
}
