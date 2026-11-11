#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>

constexpr std::byte flag_bit(int index) { return std::byte{1} << index; }

bool has_flag(std::byte flags, int index) {
  return (flags & flag_bit(index)) != std::byte{0};
}

void set_flag(std::byte& flags, int index, bool enabled) {
  if (enabled) {
    flags |= flag_bit(index);
  } else {
    flags &= ~flag_bit(index);
  }
}

std::uint16_t read_u16(std::byte high, std::byte low) {
  const auto hi = std::to_integer<std::uint16_t>(high);
  const auto lo = std::to_integer<std::uint16_t>(low);
  return static_cast<std::uint16_t>((hi << 8) | lo);
}

int main() {
  std::array<std::byte, 6> header{
      std::byte{0xCA}, std::byte{0xFE}, std::byte{0x01},
      std::byte{0x05}, std::byte{0x05}, std::byte{0x00},
  };

  std::cout << "[1] header = " << std::hex << std::uppercase
            << std::setfill('0');
  for (std::size_t i = 0; i < header.size(); ++i) {
    if (i != 0) {
      std::cout << ' ';
    }
    std::cout << std::setw(2) << std::to_integer<unsigned>(header[i]);
  }
  std::cout << std::dec << std::setfill(' ') << '\n';

  const auto version = std::to_integer<unsigned>(header[2]);
  std::byte flags = header[3];

  std::cout << "[2] version = " << version << '\n';
  std::cout << "[3] compressed = " << has_flag(flags, 0)
            << ", encrypted = " << has_flag(flags, 1) << '\n';

  set_flag(flags, 1, true);
  set_flag(flags, 0, false);
  std::cout << "[4] flags = " << std::to_integer<unsigned>(flags)
            << ", compressed = " << has_flag(flags, 0)
            << ", encrypted = " << has_flag(flags, 1) << '\n';

  std::cout << "[5] length = " << read_u16(header[4], header[5]) << '\n';
  std::cout << "[6] sizeof(byte) = " << sizeof(std::byte) << '\n';
}
