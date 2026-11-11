#include <algorithm>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct Tracked {
  Tracked() = default;
  Tracked(const Tracked&) { ++copies; }

  static int copies;
};

int Tracked::copies = 0;

int main() {
  std::vector<int> numbers{1, 2, 3};
  std::vector<std::string> words{"a", "bb"};

  auto print = [](const auto& range) {
    bool first = true;
    for (const auto& value : range) {
      if (!first) {
        std::cout << ' ';
      }
      std::cout << value;
      first = false;
    }
    std::cout << '\n';
  };

  std::cout << "[1] ints = ";
  print(numbers);
  std::cout << "[1] words = ";
  print(words);

  auto twice = [](auto value) { return value + value; };
  std::cout << "[2] twice(4) = " << twice(4)
            << ", twice(ab) = " << twice(std::string("ab")) << '\n';

  auto forward = [](auto&& value) -> decltype(auto) {
    return std::forward<decltype(value)>(value);
  };
  static_assert(std::is_same<decltype(forward(9)), int&&>::value,
                "forward keeps rvalue reference");
  int x = 7;
  int& ref = forward(x);
  ref += 1;
  std::cout << "[3] x = " << x << ", forward(9) = " << forward(9) << '\n';

  auto show = [](const auto&... values) {
    const char* sep = "";
    int unused[] = {0, ((std::cout << sep << values, sep = ", "), 0)...};
    (void)unused;
  };
  auto pack_size = [](const auto&... values) { return sizeof...(values); };
  std::cout << "[4] pack = ";
  show(1, 2.5, std::string("atlas"));
  std::cout << '\n';
  std::cout << "[4] pack size = " << pack_size(1, 2.5, std::string("atlas"))
            << '\n';

  Tracked item;
  auto by_value = [](auto value) { (void)value; };
  by_value(item);
  const int value_copies = Tracked::copies;
  Tracked::copies = 0;
  auto by_ref = [](const auto&) {};
  by_ref(item);
  std::cout << "[5] copies by value = " << value_copies
            << ", by const ref = " << Tracked::copies << '\n';
}
