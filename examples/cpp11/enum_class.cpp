#include <iostream>

enum class Color : unsigned char { red = 1, green, blue };
enum class Status { ok = 200, not_found = 404 };

const char* name(Color color) {
  switch (color) {
    case Color::red:
      return "red";
    case Color::green:
      return "green";
    case Color::blue:
      return "blue";
  }
  return "unknown";
}

int main() {
  Color color = Color::green;
  Status status = Status::not_found;

  std::cout << static_cast<int>(color) << ' ' << name(color) << ' '
            << static_cast<int>(status) << '\n';
  std::cout << (color == Color::green) << ' ' << (status == Status::ok) << '\n';

  Color next = static_cast<Color>(static_cast<unsigned char>(color) + 1);
  std::cout << name(next) << ' ' << sizeof(Color) << ' '
            << static_cast<int>(next) << '\n';
}
