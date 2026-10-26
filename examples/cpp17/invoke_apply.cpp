#include <array>
#include <functional>
#include <iostream>
#include <tuple>

struct Counter {
  int value;
  int get() const { return value; }
};

int main() {
  Counter counter{7};

  int (Counter::*getter)() const = &Counter::get;
  int Counter::*member = &Counter::value;
  std::cout << std::invoke(getter, counter) << ' '
            << std::invoke(member, counter) << '\n';

  auto args = std::make_tuple(2, 3);
  std::cout << std::apply([](int a, int b) { return a * b; }, args) << '\n';

  std::array triple{1, 2, 3};
  std::cout << std::apply([](int a, int b, int c) { return a + b + c; },
                          triple)
            << '\n';
}
