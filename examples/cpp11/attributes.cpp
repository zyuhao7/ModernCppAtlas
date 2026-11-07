#include <iostream>
#include <stdexcept>

[[noreturn]] void fail(const char* message) {
  throw std::runtime_error(message);
}

int main() {
  try {
    fail("boom");
  } catch (const std::exception& error) {
    std::cout << error.what() << '\n';
  }
}
