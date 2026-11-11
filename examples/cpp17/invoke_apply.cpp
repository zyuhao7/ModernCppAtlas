#include <array>
#include <functional>
#include <iostream>
#include <tuple>

struct Counter {
  int value;
  int get() const { return value; }
  void add(int amount) { value += amount; }
};

int max_of(int a, int b) { return a > b ? a : b; }

int main() {
  Counter counter{7};

  int (Counter::*getter)() const = &Counter::get;
  int Counter::* member = &Counter::value;
  std::cout << "[1] member call = " << std::invoke(getter, counter) << '\n';
  std::cout << "[2] member data = " << std::invoke(member, counter) << '\n';

  auto args = std::make_tuple(2, 3);
  std::cout << "[3] tuple product = "
            << std::apply([](int a, int b) { return a * b; }, args) << '\n';

  std::array triple{1, 2, 3};
  std::cout << "[4] array sum = "
            << std::apply([](int a, int b, int c) { return a + b + c; }, triple)
            << '\n';

  std::invoke(&Counter::add, std::ref(counter), 1);
  std::cout << "[5] after add = " << counter.value << '\n';

  int (*free_fn)(int, int) = &max_of;
  std::cout << "[6] free call = " << std::invoke(free_fn, 4, 9) << '\n';
}
