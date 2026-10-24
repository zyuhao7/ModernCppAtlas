#include <cstddef>
#include <iostream>

int main() {
  std::byte flags{0b0000'0110};
  flags |= std::byte{0b0000'0001};
  flags &= ~std::byte{0b0000'0010};

  std::cout << std::to_integer<unsigned>(flags) << '\n';
  std::cout << sizeof(std::byte) << '\n';
}
