#include <algorithm>
#include <compare>
#include <cstddef>
#include <iostream>
#include <limits>
#include <vector>

struct Version {
  int major;
  int minor;
  int patch;

  auto operator<=>(const Version&) const = default;
};

std::ostream& operator<<(std::ostream& os, const Version& version) {
  return os << version.major << '.' << version.minor << '.' << version.patch;
}

struct Measurement {
  double value;

  auto operator<=>(const Measurement&) const = default;
};

int main() {
  const Version a{1, 4, 0};
  const Version b{1, 10, 0};

  std::cout << (a < b) << '\n';
  std::cout << (a == b) << '\n';
  std::cout << std::is_lt(a <=> b) << '\n';

  std::vector<Version> versions{{2, 0, 0}, {1, 10, 0}, {1, 4, 1}};
  std::ranges::sort(versions);
  for (std::size_t i = 0; i < versions.size(); ++i) {
    if (i != 0) std::cout << ' ';
    std::cout << versions[i];
  }
  std::cout << '\n';

  const Measurement x{1.0};
  const Measurement y{2.0};
  std::cout << (x < y) << '\n';

  const double nan = std::numeric_limits<double>::quiet_NaN();
  const Measurement unknown{nan};
  std::cout << (unknown < y) << ' ' << (unknown > y) << ' '
            << std::is_eq(unknown <=> unknown) << '\n';
}
