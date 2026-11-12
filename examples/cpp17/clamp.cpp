#include <algorithm>
#include <cstddef>
#include <iostream>

struct Version {
  int major;
  int minor;
};

bool operator<(const Version& a, const Version& b) {
  return a.major != b.major ? a.major < b.major : a.minor < b.minor;
}

std::ostream& operator<<(std::ostream& os, const Version& version) {
  return os << version.major << '.' << version.minor;
}

int main() {
  std::cout << "[1] high = " << std::clamp(15, 0, 10)
            << ", low = " << std::clamp(-5, 0, 10)
            << ", inside = " << std::clamp(5, 0, 10) << '\n';

  std::cout << "[2] " << std::clamp(-0.5, 0.0, 1.0) << ' '
            << std::clamp(1.5, 0.0, 1.0) << ' ' << std::clamp(0.25, 0.0, 1.0)
            << '\n';

  const std::size_t size = std::clamp<std::size_t>(7, 1, 4);
  std::cout << "[3] size = " << size << '\n';

  const int raw = 15;
  std::cout << "[4] clamp = " << std::clamp(raw, 0, 10)
            << ", min(max()) = " << std::min(std::max(raw, 0), 10) << '\n';

  const Version clamped =
      std::clamp(Version{2, 5}, Version{1, 0}, Version{2, 0});
  std::cout << "[5] version = " << clamped << '\n';

  const int samples[] = {-12, 0, 37, 100, 140};
  std::cout << "[6] samples =";
  for (int sample : samples) std::cout << ' ' << std::clamp(sample, 0, 100);
  std::cout << '\n';
}
