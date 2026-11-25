#include <iostream>
#include <type_traits>

int main() {
  constexpr bool int_and_signed =
      std::conjunction_v<std::is_integral<int>, std::is_signed<int>>;
  std::cout << "[1] conjunction<is_integral<int>, is_signed<int>> = "
            << int_and_signed << '\n';

  constexpr bool double_and_signed =
      std::conjunction_v<std::is_integral<double>, std::is_signed<double>>;
  std::cout << "[2] conjunction<is_integral<double>, is_signed<double>> = "
            << double_and_signed << '\n';

  constexpr bool int_or_double =
      std::disjunction_v<std::is_integral<int>, std::is_floating_point<int>>;
  std::cout << "[3] disjunction<is_integral<int>, is_floating_point<int>> = "
            << int_or_double << '\n';

  constexpr bool not_pointer = std::negation_v<std::is_pointer<int>>;
  std::cout << "[4] negation<is_pointer<int>> = " << not_pointer << '\n';

  static_assert(std::conjunction_v<std::is_integral<int>, std::is_signed<int>>);
  static_assert(
      !std::disjunction_v<std::is_pointer<int>, std::is_reference<int>>);
}
