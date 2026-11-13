#include <charconv>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <system_error>

int main() {
  // [1] format an integer into a caller-provided buffer
  char buf[32];
  auto [p1, e1] = std::to_chars(buf, buf + sizeof(buf), 12345);
  std::cout << "[1] " << std::string(buf, p1) << '\n';

  // [2] base 16 with an explicit radix
  auto [p2, e2] = std::to_chars(buf, buf + sizeof(buf), 255, 16);
  std::cout << "[2] " << std::string(buf, p2) << '\n';

  // [3] round-trip: parse the bytes back with from_chars
  int value = 0;
  auto [p3, e3] = std::from_chars(buf, p2, value, 16);
  std::cout << "[3] parsed = " << value << ", consumed = " << (p3 == p2)
            << '\n';

  // [4] from_chars reports trailing garbage via the returned pointer
  const char* text = "42abc";
  int n = 0;
  auto [p4, e4] = std::from_chars(text, text + std::strlen(text), n);
  std::cout << "[4] value = " << n << ", stopped at '" << *p4 << "'\n";

  // [5] a malformed input sets the error code, no exception thrown
  const char* bad = "xyz";
  int m = 0;
  auto [p5, e5] = std::from_chars(bad, bad + 3, m);
  std::cout << "[5] error = " << std::boolalpha
            << (e5 == std::errc::invalid_argument) << '\n';

  // [6] floating point with fixed precision keeps every digit
  double pi = 3.14159265358979;
  auto [p6, e6] = std::to_chars(buf, buf + sizeof(buf), pi);
  std::cout << "[6] " << std::string(buf, p6) << '\n';
}
