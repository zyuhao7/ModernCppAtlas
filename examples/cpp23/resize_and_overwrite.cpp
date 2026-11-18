#include <cstdio>
#include <iostream>
#include <string>
#include <version>

int main() {
  std::string letters;
  letters.resize_and_overwrite(5, [](char* p, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      p[i] = static_cast<char>('a' + i);
    }
    return n;
  });
  std::cout << "[1] " << letters << '\n';

  std::string number;
  number.resize_and_overwrite(16, [](char* p, std::size_t n) {
    const int written = std::snprintf(p, n, "%d", 12345);
    return written < 0 ? std::size_t{0} : static_cast<std::size_t>(written);
  });
  std::cout << "[2] " << number << '\n';

  std::string shrink{"xxxxxxxx"};
  shrink.resize_and_overwrite(8, [](char* p, std::size_t) {
    p[0] = 'o';
    p[1] = 'k';
    return std::size_t{2};
  });
  std::cout << "[3] " << shrink << " size=" << shrink.size() << '\n';

  std::string alternating;
  alternating.resize_and_overwrite(6, [](char* p, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      p[i] = (i % 2 == 0) ? 'a' : 'b';
    }
    return n;
  });
  std::cout << "[4] " << alternating << '\n';

  std::string empty;
  empty.resize_and_overwrite(0,
                             [](char*, std::size_t) { return std::size_t{0}; });
  std::cout << "[5] empty size = " << empty.size() << '\n';

  std::cout << "[6] feature = " << __cpp_lib_string_resize_and_overwrite
            << '\n';
}
