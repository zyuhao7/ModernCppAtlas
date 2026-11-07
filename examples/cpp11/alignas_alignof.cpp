#include <cstdint>
#include <iostream>

struct alignas(16) Vec4 {
  float values[4];
};

int main() {
  std::cout << alignof(Vec4) << ' ' << sizeof(Vec4) << '\n';
  alignas(32) int buffer[8] = {};
  std::cout << (reinterpret_cast<std::uintptr_t>(buffer) % 32 == 0) << '\n';
}
