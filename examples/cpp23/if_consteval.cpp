#include <array>
#include <iostream>

constexpr int branch(int input) {
  if consteval {
    return input + 1;
  } else {
    return input + 2;
  }
}

constexpr int sum(const std::array<int, 4>& a) {
  if consteval {
    return a[0] + a[1] + a[2] + a[3];
  } else {
    int total = 0;
    for (int value : a) {
      total += value;
    }
    return total + 100;
  }
}

static_assert(branch(40) == 41);

int main() {
  constexpr int at_compile_time = branch(40);
  std::cout << "[1] compile-time branch = " << at_compile_time << '\n';

  volatile int seed = 40;
  const int at_runtime = branch(seed);
  std::cout << "[2] runtime branch = " << at_runtime << '\n';

  constexpr std::array<int, 4> values{1, 2, 3, 4};
  constexpr int compile_sum = sum(values);
  static_assert(compile_sum == 10);
  std::cout << "[3] constexpr sum = " << compile_sum << '\n';

  std::array<int, 4> runtime_values{1, 2, 3, 4};
  std::cout << "[4] runtime sum = " << sum(runtime_values) << '\n';

  std::cout << "[5] __cpp_if_consteval = " << __cpp_if_consteval << '\n';
}
