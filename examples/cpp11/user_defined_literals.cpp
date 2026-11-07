#include <cstddef>
#include <iostream>
#include <string>

struct Kilometers {
  double value;
};

Kilometers operator""_km(long double value) {
  return Kilometers{static_cast<double>(value)};
}

std::string operator""_quoted(const char* text, std::size_t size) {
  return std::string("\"") + std::string(text, size) + "\"";
}

int main() {
  Kilometers distance = 12.5_km;
  std::cout << distance.value << '\n';
  std::cout << "atlas"_quoted << '\n';
}
