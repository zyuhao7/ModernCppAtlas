#include <array>
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

const int* first_ref() { return &config::kMaxRetries; }
const int* second_ref() { return &config::kMaxRetries; }

void bump() { Limits::calls += 1; }

int main() {
  const int* first = &config::kMaxRetries;
  const int* second = &config::kMaxRetries;
  const std::array<int, config::kMaxRetries> retries{};

  std::cout << "[1] same address = " << (first == second)
            << ", value = " << config::kMaxRetries << '\n';

  std::cout << "[2] array size = " << retries.size()
            << ", max = " << Limits::kMax << ", calls = " << Limits::calls
            << '\n';

  std::cout << "[3] service = " << config::kServiceName
            << ", size = " << config::kServiceName.size() << '\n';

  std::cout << "[4] cross function same = " << (first_ref() == second_ref())
            << '\n';

  Limits::calls += 1;
  bump();
  std::cout << "[5] calls after bump = " << Limits::calls << '\n';
}
