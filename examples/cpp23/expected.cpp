#include <expected>
#include <iostream>
#include <string>
#include <string_view>

std::expected<int, std::string> parse_port(std::string_view text) {
  if (text.empty()) {
    return std::unexpected(std::string{"empty port"});
  }
  int value = 0;
  for (char c : text) {
    if (c < '0' || c > '9') {
      return std::unexpected(std::string{"port must be numeric"});
    }
    value = value * 10 + (c - '0');
  }
  if (value > 65535) {
    return std::unexpected(std::string{"port out of range"});
  }
  return value;
}

std::expected<int, std::string> doubled(std::string_view text) {
  return parse_port(text).transform([](int port) { return port * 2; });
}

std::expected<int, std::string> with_default(std::string_view text) {
  return parse_port(text).or_else(
      [](const std::string&) -> std::expected<int, std::string> {
        return 8080;
      });
}

std::expected<void, std::string> check_bounds(int port) {
  if (port < 1024) {
    return std::unexpected(std::string{"privileged port"});
  }
  return {};
}

int main() {
  // 成功与失败共用同一个返回类型，调用方先检查再解引用。
  for (std::string_view text : {"8080", "abc", "70000"}) {
    const auto result = parse_port(text);
    if (result) {
      std::cout << "[1] " << text << " -> port " << *result << '\n';
    } else {
      std::cout << "[1] " << text << " -> error: " << result.error() << '\n';
    }
  }

  // value() 在错误状态下抛出 bad_expected_access。
  try {
    (void)parse_port("bad").value();
  } catch (const std::bad_expected_access<std::string>& error) {
    std::cout << "[2] value() threw, error = " << error.error() << '\n';
  }

  // value_or 提供兜底值，不区分失败原因。
  std::cout << "[3] value_or(80) = " << parse_port("abc").value_or(80)
            << ", value_or(80) on success = " << parse_port("9000").value_or(80)
            << '\n';

  // transform 在失败时短路，回调不会执行。
  std::cout << "[4] doubled(21) = " << *doubled("21")
            << ", doubled(bad) has value = " << doubled("bad").has_value()
            << ", error = " << doubled("bad").error() << '\n';

  // or_else 把错误映射成兜底成功值。
  std::cout << "[5] default(abc) = " << *with_default("abc")
            << ", default(1234) = " << *with_default("1234") << '\n';

  // expected<void, E> 只表达成功或失败，没有成功值。
  std::cout << "[6] check(80) ok = " << check_bounds(80).has_value()
            << ", check(8080) ok = " << check_bounds(8080).has_value()
            << ", error = " << check_bounds(80).error() << '\n';
}
