#include <iostream>
#include <string_view>

namespace config {
inline constexpr int kMaxRetries = 3;
inline constexpr std::string_view kServiceName = "atlas";
}  // namespace config

struct Limits {
  static inline int calls = 0;
  static constexpr int kMax = 3;
};

int main() {
  const int *first = &config::kMaxRetries;
  const int *second = &config::kMaxRetries;
  Limits::calls += 1;

  std::cout << (first == second) << ' ' << config::kMaxRetries << ' '
            << Limits::calls << ' ' << Limits::kMax << ' '
            << config::kServiceName << '\n';
}
