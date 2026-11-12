#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <version>

#if defined(__cpp_lib_expected)
#include <expected>
#endif

#if defined(__cpp_lib_expected)

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

#else

// Clang 18 配合 libstdc++ 13 时，<expected> 的声明被 __cpp_concepts 挡在门外。
// 回退分支用普通结构体重跑同样的六个场景，保证快照在两条工具链上逐字一致。

struct PortResult {
  bool ok;
  int number;
  std::string message;

  PortResult(int value) : ok(true), number(value) {}
  PortResult(std::string error)
      : ok(false), number(0), message(std::move(error)) {}

  bool has_value() const { return ok; }

  int value() const {
    if (!ok) throw std::runtime_error(message);
    return number;
  }

  const std::string& error() const { return message; }
  int value_or(int fallback) const { return ok ? number : fallback; }
};

PortResult parse_port(std::string_view text) {
  if (text.empty()) return std::string{"empty port"};

  int value = 0;
  for (char c : text) {
    if (c < '0' || c > '9') return std::string{"port must be numeric"};
    value = value * 10 + (c - '0');
  }
  if (value > 65535) return std::string{"port out of range"};
  return value;
}

PortResult doubled(std::string_view text) {
  PortResult result = parse_port(text);
  if (result.has_value()) result.number *= 2;
  return result;
}

PortResult with_default(std::string_view text) {
  PortResult result = parse_port(text);
  if (!result.has_value()) return 8080;
  return result;
}

PortResult check_bounds(int port) {
  if (port < 1024) return std::string{"privileged port"};
  return 0;
}

int main() {
  for (std::string_view text : {"8080", "abc", "70000"}) {
    const PortResult result = parse_port(text);
    if (result.has_value()) {
      std::cout << "[1] " << text << " -> port " << result.value() << '\n';
    } else {
      std::cout << "[1] " << text << " -> error: " << result.error() << '\n';
    }
  }

  try {
    (void)parse_port("bad").value();
  } catch (const std::runtime_error& error) {
    std::cout << "[2] value() threw, error = " << error.what() << '\n';
  }

  std::cout << "[3] value_or(80) = " << parse_port("abc").value_or(80)
            << ", value_or(80) on success = " << parse_port("9000").value_or(80)
            << '\n';

  std::cout << "[4] doubled(21) = " << doubled("21").value()
            << ", doubled(bad) has value = " << doubled("bad").has_value()
            << ", error = " << doubled("bad").error() << '\n';

  std::cout << "[5] default(abc) = " << with_default("abc").value()
            << ", default(1234) = " << with_default("1234").value() << '\n';

  std::cout << "[6] check(80) ok = " << check_bounds(80).has_value()
            << ", check(8080) ok = " << check_bounds(8080).has_value()
            << ", error = " << check_bounds(80).error() << '\n';
}

#endif
