#include <compare>
#include <iostream>

struct Version {
  int major;
  int minor;
  int patch;

  auto operator<=>(const Version&) const = default;
};

int main() {
  Version a{1, 4, 0};
  Version b{1, 10, 0};
  std::cout << (a < b) << '\n';
  std::cout << (a == b) << '\n';
}
