#include <array>
#include <iostream>
#include <list>
#include <ranges>
#include <vector>
#include <version>

int main() {
  std::array<int, 4> source{{1, 2, 3, 4}};

  // The tagged constructor consumes a whole range; libraries without full
  // C++23 container support fall back to the iterator-pair form.
#ifdef __cpp_lib_containers_ranges
  std::vector<int> copied(std::from_range, source);
  std::list<int> chained(std::from_range, copied);
#else
  std::vector<int> copied(source.begin(), source.end());
  std::list<int> chained(copied.begin(), copied.end());
#endif

  std::cout << "[1] vector from array =";
  for (int value : copied) std::cout << ' ' << value;
  std::cout << '\n';

  std::cout << "[2] vector size = " << copied.size() << '\n';

  std::cout << "[3] list from vector =";
  for (int value : chained) std::cout << ' ' << value;
  std::cout << '\n';

  std::cout << "[4] list front/back = " << chained.front() << ' '
            << chained.back() << '\n';
}
