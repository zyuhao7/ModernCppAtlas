#include <chrono>
#include <iostream>
#include <ratio>

int main() {
  // ratio normalizes the fraction at compile time: 2/4 becomes 1/2.
  using half = std::ratio<2, 4>;
  std::cout << "[1] ratio<2, 4> = " << half::num << '/' << half::den << '\n';

  using total = std::ratio_add<std::ratio<1, 2>, std::ratio<1, 4>>;
  std::cout << "[2] ratio_add<1/2, 1/4> = " << total::num << '/' << total::den
            << '\n';

  using scale = std::ratio_divide<std::milli, std::micro>;
  std::cout << "[3] ratio_divide<milli, micro> = " << scale::num << '/'
            << scale::den << '\n';

  std::cout << "[4] milli < centi = "
            << std::ratio_less<std::milli, std::centi>::value << '\n';
  std::cout << "[5] milli == ratio<1, 1000> = "
            << std::ratio_equal<std::milli, std::ratio<1, 1000>>::value << '\n';

  // chrono periods are ratios; duration_cast uses them to convert units.
  using milliseconds = std::chrono::duration<long long, std::milli>;
  auto elapsed =
      std::chrono::duration_cast<std::chrono::seconds>(milliseconds(1500));
  std::cout << "[6] 1500 ms as seconds = " << elapsed.count() << '\n';
}
