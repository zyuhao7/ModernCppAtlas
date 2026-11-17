#include <cmath>
#include <iostream>

int main() {
  // [1] classic interpolation at t = 0.25
  std::cout << "[1] " << std::lerp(0.0, 10.0, 0.25) << '\n';

  // [2] the endpoints reproduce the inputs
  std::cout << "[2] t=0 -> " << std::lerp(0.0, 10.0, 0.0) << ", t=1 -> "
            << std::lerp(0.0, 10.0, 1.0) << '\n';

  // [3] t outside [0, 1] extrapolates linearly
  std::cout << "[3] t=2 -> " << std::lerp(0.0, 10.0, 2.0) << '\n';

  // [4] works for descending ranges too
  std::cout << "[4] " << std::lerp(10.0, -10.0, 0.75) << '\n';

  // [5] constexpr since C++20; midpoint stays exact here
  constexpr double mid = std::lerp(1.0, 2.0, 0.5);
  static_assert(mid == 1.5);
  std::cout << "[5] constexpr mid = " << mid << '\n';

  // [6] a larger ascending range at the midpoint
  std::cout << "[6] " << std::lerp(10.0, 20.0, 0.5) << '\n';
}
