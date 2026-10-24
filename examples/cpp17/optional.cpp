#include <charconv>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>

using Config = std::map<std::string, std::string>;

std::optional<int> parse_int(std::string_view text) {
  int value = 0;
  auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || ptr != text.data() + text.size()) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::string_view> find_value(const Config& config,
                                           std::string_view key) {
  const auto it = config.find(std::string(key));
  if (it == config.end()) return std::nullopt;
  return std::string_view(it->second);
}

int main() {
  const Config config{
      {"host", "127.0.0.1"},
      {"port", "8080"},
      {"debug", "false"},
  };

  const auto host = find_value(config, "host");
  std::cout << host.value_or("<unset>") << '\n';

  const auto port = parse_int(find_value(config, "port").value_or(""));
  std::cout << port.value_or(80) << '\n';

  std::cout << parse_int("nope").value_or(-1) << '\n';
  std::cout << find_value(config, "timeout").value_or("30") << '\n';

  std::optional<bool> enabled = false;
  std::cout << "present: " << enabled.has_value() << ", enabled: " << *enabled
            << '\n';

  std::optional<std::string> label;
  label.emplace("atlas");
  std::cout << label.value() << '\n';

  label.reset();
  std::cout << label.value_or("none") << '\n';
}
