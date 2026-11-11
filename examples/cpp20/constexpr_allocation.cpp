#include <array>
#include <iostream>
#include <type_traits>

constexpr int sum_squares(int n) {
  int* values = new int[n];
  for (int i = 0; i < n; ++i) {
    values[i] = i * i;
  }
  int sum = 0;
  for (int i = 0; i < n; ++i) {
    sum += values[i];
  }
  delete[] values;
  return sum;
}

consteval std::array<int, 4> squares_table() {
  int* values = new int[4];
  for (int i = 0; i < 4; ++i) {
    values[i] = i * i;
  }
  std::array<int, 4> result{};
  for (int i = 0; i < 4; ++i) {
    result[i] = values[i];
  }
  delete[] values;
  return result;
}

constexpr int mode(int value) {
  if (std::is_constant_evaluated()) {
    return value;
  }
  return value + 1;
}

int main() {
  constexpr int compile_time = sum_squares(4);
  const int run_time = sum_squares(5);
  std::cout << "[1] compile-time sum = " << compile_time
            << ", runtime sum = " << run_time << '\n';

  constexpr auto table = squares_table();
  static_assert(table[3] == 9, "table is built at compile time");
  std::cout << "[2] squares = " << table[0] << ' ' << table[1] << ' '
            << table[2] << ' ' << table[3] << '\n';

  int runtime_n = 6;
  std::cout << "[3] runtime n = " << runtime_n << " -> "
            << sum_squares(runtime_n) << '\n';

  constexpr int compile_mode = mode(1);
  int runtime_value = 1;
  const int runtime_mode = mode(runtime_value);
  std::cout << "[4] compile mode = " << compile_mode
            << ", runtime mode = " << runtime_mode << '\n';

  static_assert(compile_time == 14 && sum_squares(3) == 5);
  std::cout << "[5] static_assert values = " << compile_time << ' '
            << sum_squares(3) << '\n';
}
