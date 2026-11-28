#include <cstdint>
#include <iostream>

int main() {
  // Binary literals keep the bit layout visible instead of hiding it in hex.
  const std::uint8_t flags = 0b00001010;
  std::cout << "[1] flags = " << static_cast<int>(flags) << '\n';

  const std::uint8_t mask = 0b11110000;
  std::cout << "[2] mask = " << static_cast<int>(mask) << '\n';

  const std::uint8_t byte = 0b10110110;
  std::cout << "[3] low nibble = " << static_cast<int>(byte & 0b00001111)
            << '\n';

  const std::uint16_t utf8_tail = 0b10000000;
  std::cout << "[4] utf8 tail = 0x" << std::hex << utf8_tail << '\n';

  switch (byte >> 6) {
    case 0b10:
      std::cout << "[5] leading bits = two\n";
      break;
    default:
      std::cout << "[5] leading bits = other\n";
      break;
  }
}
