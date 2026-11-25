#include <cstddef>
#include <iostream>

int classify(int value) {
  if (value > 0) [[likely]] {
    return 1;
  }
  if (value < 0) [[unlikely]] {
    return -1;
  }
  return 0;
}

struct Empty {};

struct Plain {
  int value;
  Empty tag;
};

struct Compact {
  int value;
  [[no_unique_address]] Empty tag;
};

int main() {
  std::cout << "[1] classify(-3) = " << classify(-3) << '\n';
  std::cout << "[2] classify(0) = " << classify(0) << '\n';
  std::cout << "[3] classify(5) = " << classify(5) << '\n';
  std::cout << "[4] sizeof(Plain) = " << sizeof(Plain)
            << ", sizeof(Compact) = " << sizeof(Compact) << '\n';
}
