#include <iostream>
#include <type_traits>

namespace {

constexpr int evaluate(int value) {
  if (std::is_constant_evaluated()) return value * 2;
  return value + 1;
}

constexpr int path_id() { return std::is_constant_evaluated() ? 1 : 2; }

}  // namespace

int main() {
  // [1] constant evaluation takes the compile-time branch
  constexpr int compile_time = evaluate(10);
  static_assert(compile_time == 20);
  std::cout << "[1] compile-time = " << compile_time << '\n';

  // [2] the same function called with a runtime value takes the other branch
  int value = 10;
  std::cout << "[2] runtime = " << evaluate(value) << '\n';

  // [3] a manifestly constant-evaluated initializer reports true
  constexpr bool in_constant = std::is_constant_evaluated();
  std::cout << "[3] constexpr initializer = " << std::boolalpha << in_constant
            << '\n';

  // [4] an ordinary initializer reports false
  bool at_runtime = std::is_constant_evaluated();
  std::cout << "[4] runtime call = " << at_runtime << '\n';

  // [5] a constexpr initializer records the compile-time path id
  constexpr int compile_path = path_id();
  static_assert(compile_path == 1);
  std::cout << "[5] compile-time path = " << compile_path << '\n';

  // [6] the runtime call returns the other id
  std::cout << "[6] runtime path = " << path_id() << '\n';
}
