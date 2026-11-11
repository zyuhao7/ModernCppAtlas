#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

template <typename... Args>
auto sum(const Args&... args) {
  return (args + ... + 0);
}

template <typename... Preds>
bool all_true(Preds... preds) {
  return (preds && ...);
}

template <typename... Preds>
bool any_true(Preds... preds) {
  return (preds || ...);
}

template <typename... Args>
void print_all(const Args&... args) {
  (std::cout << ... << args) << '\n';
}

template <typename F, typename... Args>
void for_each(F f, Args&&... args) {
  (f(std::forward<Args>(args)), ...);
}

template <typename... Ts>
constexpr bool all_integral = (std::is_integral_v<Ts> && ...);

template <typename... Args>
constexpr std::size_t count_args(const Args&...) {
  return sizeof...(Args);
}

static_assert(all_integral<int, long, unsigned char>);
static_assert(!all_integral<int, double>);

int main() {
  std::cout << "[1] sum = " << sum(1, 2, 3, 4, 5) << ", empty = " << sum()
            << '\n';
  std::cout << "[2] all = " << all_true(true, 2 < 3, 4 == 4)
            << ", all empty = " << all_true()
            << ", any = " << any_true(false, false, 3 < 4)
            << ", any empty = " << any_true() << '\n';

  std::cout << "[3] ";
  print_all("joined: ", 1, ' ', 2.5, ' ', "atlas");

  int total = 0;
  for_each([&total](int value) { total += value; }, 10, 20, 30);
  std::cout << "[4] total = " << total << '\n';

  std::cout << "[5] all integral = "
            << all_integral<int, long, unsigned char> << ", mixed integral = "
            << all_integral<int, double> << '\n';

  std::cout << "[6] count = " << count_args(1, std::string("x"), 3.5) << '\n';
}
