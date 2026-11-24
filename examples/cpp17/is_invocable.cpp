#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

int twice(int value) { return value * 2; }

std::string join(const std::string& a, const std::string& b) { return a + b; }

struct Accumulator {
  int total = 0;
  int add(int value) { return total += value; }
};

template <typename Fn, typename... Args>
constexpr bool callable_with = std::is_invocable_v<Fn, Args...>;

int main() {
  std::cout
      << "[1] twice(int) = "
      << std::is_invocable_v<decltype(&twice), int> << ", twice(string) = "
      << std::is_invocable_v<decltype(&twice), std::string> << '\n';

  std::cout << "[2] join(as, as) = "
            << std::is_invocable_v<decltype(&join), const std::string&,
                                   const std::string&> << ", join(int, int) = "
            << std::is_invocable_v<decltype(&join), int, int> << '\n';

  std::cout << "[3] add(int) member = "
            << std::is_invocable_v<decltype(&Accumulator::add), Accumulator&,
                                   int> << '\n';

  int captured = 5;
  auto lambda = [captured](int v) { return captured + v; };
  std::cout << "[4] lambda(int) = "
            << std::is_invocable_v<decltype(lambda), int> << ", lambda() = "
            << std::is_invocable_v<decltype(lambda)> << '\n';

  std::cout << "[5] invoke_result_t<twice, int> is int = "
            << std::is_same_v<std::invoke_result_t<decltype(&twice), int>,
                              int> << '\n';

  std::cout
      << "[6] is_invocable_r<int, twice, int> = "
      << std::is_invocable_r_v<int, decltype(&twice),
                               int> << ", is_invocable_r<double, twice, int> = "
      << std::is_invocable_r_v<double, decltype(&twice), int> << '\n';

  static_assert(callable_with<decltype(&twice), int>);
  static_assert(!callable_with<decltype(&twice), double, double>);
  static_assert(callable_with<Accumulator, std::vector<int>> == false);
}
