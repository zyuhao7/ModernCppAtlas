#include <bit>
#include <cstdint>
#include <iomanip>
#include <iostream>

int main() {
  std::cout << std::hex << std::setfill('0');

  // [1] 16-bit reversal: 0x1234 -> 0x3412
  std::uint16_t a = 0x1234;
  std::cout << "[1] " << std::setw(4) << std::byteswap(a) << '\n';

  // [2] 32-bit reversal: byte order becomes 04 03 02 01
  std::uint32_t b = 0x01020304;
  std::cout << "[2] " << std::setw(8) << std::byteswap(b) << '\n';

  // [3] swapping twice restores the original value
  std::uint32_t c = 0xDEADBEEF;
  std::cout << "[3] round-trip = " << std::boolalpha
            << (std::byteswap(std::byteswap(c)) == c) << '\n';

  // [4] a single-byte integer is unchanged
  std::uint8_t d = 0xAB;
  std::cout << "[4] " << std::setw(2) << static_cast<int>(std::byteswap(d))
            << '\n';

  // [5] constexpr-friendly: usable in constant expressions
  constexpr std::uint32_t host = 0x01020304;
  constexpr std::uint32_t net = std::byteswap(host);
  static_assert(net == 0x04030201);
  std::cout << "[5] big-endian value = " << std::setw(8) << net << '\n';

  // [6] trailing/leading zero bytes move across
  std::uint16_t e = 0x00FF;
  std::cout << "[6] " << std::setw(4) << std::byteswap(e) << '\n';

  std::cout << std::dec;
}
