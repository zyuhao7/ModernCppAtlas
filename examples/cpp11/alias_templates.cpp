#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

template <typename T>
using Matrix = std::vector<std::vector<T>>;

template <typename Key, typename Value>
using Table = std::unordered_map<Key, Value>;

template <typename T>
using remove_cvref_t =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;

int main() {
  Matrix<int> grid{{1, 2}, {3, 4}};
  Table<std::string, int> ages{{"atlas", 3}, {"modern", 11}};

  std::cout << grid.size() << ' ' << grid[1][1] << '\n';
  std::cout << ages.size() << ' ' << ages.at("modern") << '\n';

  static_assert(std::is_same<remove_cvref_t<const int&>, int>::value,
                "alias template strips cv and reference");
  std::cout << std::is_same<remove_cvref_t<const int&>, int>::value << '\n';
}
