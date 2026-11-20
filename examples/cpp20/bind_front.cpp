#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <utility>

struct Counter {
  int total = 0;
  void add(int value) { total += value; }
};

int main() {
  const auto subtract_from_ten =
      std::bind_front([](int left, int right) { return left - right; }, 10);
  std::cout << "[1] 10 - 3 = " << subtract_from_ten(3) << '\n';

  int total = 5;
  auto add = std::bind_front([](int& sum, int value) { sum += value; },
                             std::ref(total));
  add(4);
  assert(total == 9);
  std::cout << "[2] updated total = " << total << '\n';

  auto consume =
      std::bind_front([](std::unique_ptr<int> value) { return *value; },
                      std::make_unique<int>(42));
  // 以右值调用包装器，把其中保存的所有权移交给目标函数。
  std::cout << "[3] consumed value = " << std::move(consume)() << '\n';

  int offset = 10;
  const auto add_saved = std::bind_front(std::plus<>{}, offset);
  offset = 20;
  std::cout << "[4] bound offset + 3 = " << add_saved(3)
            << ", current offset = " << offset << '\n';

  Counter counter;
  auto add_to_counter = std::bind_front(&Counter::add, &counter);
  add_to_counter(6);
  assert(counter.total == 6);
  std::cout << "[5] counter = " << counter.total << '\n';

  const auto combine = std::bind_front(
      [](int a, int b, int c) { return 100 * a + 10 * b + c; }, 1, 2);
  std::cout << "[6] combined digits = " << combine(3) << '\n';
}
