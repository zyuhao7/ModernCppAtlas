#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace {

// Serialize an integer into a fixed byte buffer, optionally normalizing to a
// requested endianness so the result no longer depends on the host.
template <class T>
std::array<std::uint8_t, sizeof(T)> to_bytes(T value, bool big_endian) {
  std::array<std::uint8_t, sizeof(T)> out{};
  std::memcpy(out.data(), &value, sizeof(T));
  if constexpr (std::endian::native == std::endian::little) {
    if (big_endian) std::reverse(out.begin(), out.end());
  } else {
    if (!big_endian) std::reverse(out.begin(), out.end());
  }
  return out;
}

void print(const std::array<std::uint8_t, 4>& bytes) {
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    if (i) std::cout << ' ';
    std::cout << static_cast<int>(bytes[i]);
  }
}

}  // namespace

int main() {
  // [1] native is always one of the two well-defined kinds, rejected here at
  // compile time if it were neither
  static_assert(std::endian::native == std::endian::little ||
                std::endian::native == std::endian::big);
  std::cout << "[1] native is little or big = " << std::boolalpha
            << (std::endian::native == std::endian::little ||
                std::endian::native == std::endian::big)
            << '\n';

  // [2] the enum values are distinct
  std::cout << "[2] little != big = " << std::boolalpha
            << (std::endian::little != std::endian::big) << '\n';

  // [3] normalize 0x01020304 to big-endian bytes, portable across hosts
  std::cout << "[3] big-endian = ";
  print(to_bytes<std::uint32_t>(0x01020304u, true));
  std::cout << '\n';

  // [4] same value normalized to little-endian
  std::cout << "[4] little-endian = ";
  print(to_bytes<std::uint32_t>(0x01020304u, false));
  std::cout << '\n';

  // [5] a single byte has no byte order
  std::cout << "[5] sizeof(uint8_t) = " << sizeof(std::uint8_t) << '\n';

  // [6] round-trip: a big-endian byte stream decoded back to host order
  std::array<std::uint8_t, 4> wire = {1, 2, 3, 4};
  if constexpr (std::endian::native == std::endian::little)
    std::reverse(wire.begin(), wire.end());
  std::uint32_t decoded = 0;
  std::memcpy(&decoded, wire.data(), sizeof(decoded));
  std::cout << "[6] decoded = 0x" << std::hex << decoded << '\n';
}
