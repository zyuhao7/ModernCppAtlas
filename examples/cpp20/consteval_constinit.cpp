#include <iostream>

consteval int square(int value) { return value * value; }

constinit int global_counter = square(3);

struct Token {
  int value;
  consteval explicit Token(int input) : value(square(input)) {}
};

int main() {
  static_assert(square(5) == 25, "square is evaluated at compile time");
  std::cout << "[1] square(5) = " << square(5) << '\n';

  global_counter += 1;
  std::cout << "[2] global_counter = " << global_counter << '\n';

  const Token first{4};
  const Token second{5};
  std::cout << "[3] token = " << first.value << ", second = " << second.value
            << '\n';

  static constinit int local = square(2);
  local += 3;
  std::cout << "[4] local = " << local << '\n';

  int table[square(2)] = {};
  std::cout << "[5] table size = " << sizeof(table) / sizeof(table[0]) << '\n';
}
