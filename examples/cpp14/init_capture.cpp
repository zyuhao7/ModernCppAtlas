#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

int main() {
  int base = 10;

  auto add = [offset = base / 2](int value) { return value + offset; };
  std::cout << "[1] " << add(7) << '\n';

  auto squared = [value = base * base] { return value; };
  std::cout << "[2] " << squared() << '\n';

  auto owned = std::make_unique<std::string>("payload");
  auto take = [text = std::move(owned)] {
    return text ? *text : std::string{};
  };
  std::cout << "[3] " << take() << ", source null = " << (owned == nullptr)
            << '\n';

  std::function<int(int)> scaled = [factor = 3](int value) {
    return value * factor;
  };
  std::cout << "[4] " << scaled(4) << '\n';

  auto counter = [count = 0]() mutable { return ++count; };
  counter();
  counter();
  std::cout << "[5] calls = " << counter() << '\n';

  std::vector<std::function<int()>> tasks;
  for (int i = 1; i <= 3; ++i)
    tasks.push_back([value = i, square = i * i] { return value + square; });
  std::cout << "[6]";
  for (const auto& task : tasks) std::cout << ' ' << task();
  std::cout << '\n';
}
