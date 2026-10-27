#include <iostream>

enum class Color { red, green, blue };
enum class Status { ok = 200, not_found = 404 };

int main() {
  Color color = Color::green;
  Status status = Status::not_found;
  std::cout << static_cast<int>(color) << ' ' << static_cast<int>(status)
            << '\n';
}
