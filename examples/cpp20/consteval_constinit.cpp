#include <iostream>

consteval int square(int value) { return value * value; }

constinit int global_counter = square(3);

struct Token {
  int value;
  consteval explicit Token(int input) : value(square(input)) {}
};

int main() {
  global_counter += 1;
  Token token{4};
  static_assert(square(5) == 25);
  std::cout << global_counter << ' ' << token.value << '\n';
}
