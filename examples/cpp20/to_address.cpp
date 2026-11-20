#include <cassert>
#include <iostream>
#include <memory>
#include <span>
#include <vector>

struct Pointer {
  const int* raw;
  const int* operator->() const noexcept { return raw; }
};

int main() {
  const auto owner = std::make_unique<int>(42);
  const auto* raw = std::to_address(owner);
  assert(raw == owner.get());
  std::cout << "[1] owned value = " << *raw << '\n';

  const std::vector<int> values{10, 20, 30};
  const auto* next = std::to_address(values.begin() + 1);
  assert(next == values.data() + 1);
  std::cout << "[2] second value = " << *next << '\n';

  const int* empty = nullptr;
  std::cout << "[3] null stays null = " << (std::to_address(empty) == nullptr)
            << '\n';

  const Pointer pointer{raw};
  assert(std::to_address(pointer) == raw);
  std::cout << "[4] wrapped address matches = "
            << (std::to_address(pointer) == raw) << '\n';

  const std::span<const int> tail(next, values.size() - 1);
  std::cout << "[5] tail = " << tail.front() << ' ' << tail.back() << '\n';
}
