#include <iostream>
#include <utility>

enum class Direction { north, east, south, west };

// The switch covers every enumerator, so control never reaches the call; it is
// there to tell the compiler that the fall-through path cannot happen.
int dx(Direction d) {
  switch (d) {
    case Direction::north:
      return 0;
    case Direction::east:
      return 1;
    case Direction::south:
      return 0;
    case Direction::west:
      return -1;
  }
  std::unreachable();
}

const char* name(Direction d) {
  switch (d) {
    case Direction::north:
      return "north";
    case Direction::east:
      return "east";
    case Direction::south:
      return "south";
    case Direction::west:
      return "west";
  }
  std::unreachable();
}

int main() {
  std::cout << "[1] dx east = " << dx(Direction::east) << '\n';
  std::cout << "[2] dx west = " << dx(Direction::west) << '\n';
  std::cout << "[3] name north = " << name(Direction::north) << '\n';
  std::cout << "[4] name south = " << name(Direction::south) << '\n';

  // Feature-test macro is provided by the standard library, not the compiler.
#if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
  std::cout << "[5] std::unreachable available = true\n";
#else
  std::cout << "[5] std::unreachable available = false\n";
#endif

  std::cout << "[6] sum of dx = "
            << dx(Direction::north) + dx(Direction::east) +
                   dx(Direction::south) + dx(Direction::west)
            << '\n';
}
