#include <cstdint>
#include <iostream>

enum class Mode : std::uint8_t;

enum class Mode : std::uint8_t { fast, safe };

Mode pick(bool fast) { return fast ? Mode::fast : Mode::safe; }

int main() {
  std::cout << (pick(true) == Mode::fast ? "fast" : "safe") << '\n';
  std::cout << sizeof(Mode) << '\n';
}
