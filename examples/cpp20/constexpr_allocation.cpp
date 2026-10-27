#include <iostream>

constexpr int sum_squares(int n) {
  int *values = new int[n];
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

int main() {
  constexpr int compile_time = sum_squares(4);
  const int run_time = sum_squares(5);

  std::cout << compile_time << ' ' << run_time << '\n';
}
