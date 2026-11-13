#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>

int main() {
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "[1] pi = " << std::numbers::pi << '\n';
  std::cout << "[2] e = " << std::numbers::e
            << ", sqrt2 = " << std::numbers::sqrt2 << '\n';

  std::cout << std::setprecision(8);
  std::cout << "[3] pi_v<float> = " << std::numbers::pi_v<float> << '\n';

  const double radius = 2.0;
  const double area = std::numbers::pi * radius * radius;
  std::cout << std::setprecision(5);
  std::cout << "[4] area = " << area << '\n';

  std::cout << std::setprecision(6);
  std::cout << "[5] ln10 = " << std::numbers::ln10
            << ", log10(e) = " << std::numbers::log10e << '\n';

  const double degrees = 180.0;
  const double radians = degrees * std::numbers::pi / 180.0;
  std::cout << "[6] radians = " << radians << ", cos = " << std::cos(radians)
            << '\n';
}
