#include <cstddef>
#include <iostream>
#include <string>

template <typename T>
constexpr T pi = T(3.1415926535897932385);

template <typename T>
constexpr bool is_small = sizeof(T) <= sizeof(int);

template <typename T>
constexpr bool is_string = false;

template <>
constexpr bool is_string<std::string> = true;

template <int N>
constexpr int square = N * N;

template <typename T>
T counter = T{};

int main() {
  static_assert(is_small<char>, "char fits in int");
  static_assert(!is_small<double>, "double is wider than int");
  static_assert(pi<float> == static_cast<float>(pi<double>),
                "float instance rounds");

  std::cout << "[1] pi<double> = " << pi<double> << ", pi<float> = "
            << pi<float> << '\n';
  std::cout << "[2] is_small<char> = "
            << is_small<char> << ", is_small<double> = "
            << is_small<double> << '\n';
  std::cout << "[3] is_string<int> = "
            << is_string<int> << ", is_string<std::string> = "
            << is_string<std::string> << '\n';
  std::cout << "[4] square<3> = " << square<3> << ", square<4> = "
            << square<4> << '\n';

  counter<int> += 41;
  counter<int> += 1;
  counter<double> += 1.5;
  std::cout << "[5] counter<int> = " << counter<int> << ", counter<double> = "
            << counter<double> << '\n';
}
