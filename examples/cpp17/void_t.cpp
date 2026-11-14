#include <iostream>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

template <class, class = void>
struct has_value_type : std::false_type {};

template <class T>
struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {
};

template <class, class = void>
struct has_size_method : std::false_type {};

template <class T>
struct has_size_method<T, std::void_t<decltype(std::declval<T>().size())>>
    : std::true_type {};

template <class, class = void>
struct has_reserve : std::false_type {};

template <class T>
struct has_reserve<T, std::void_t<decltype(std::declval<T&>().reserve(0))>>
    : std::true_type {};

template <class, class = void>
struct is_iterable : std::false_type {};

template <class T>
struct is_iterable<T, std::void_t<decltype(std::begin(std::declval<T&>())),
                                  decltype(std::end(std::declval<T&>()))>>
    : std::true_type {};

}  // namespace

int main() {
  // [1] a member type makes the specialization viable
  std::cout << "[1] vector has value_type = " << std::boolalpha
            << has_value_type<std::vector<int>>::value << '\n';

  // [2] a type without the member falls back to the primary template
  std::cout << "[2] int has value_type = " << has_value_type<int>::value
            << '\n';

  // [3] a member function can be detected through its call expression
  std::cout << "[3] string has size() = " << has_size_method<std::string>::value
            << '\n';

  // [4] the same expression fails for an unrelated type
  std::cout << "[4] int has size() = " << has_size_method<int>::value << '\n';

  // [5] detect a member function with arguments
  std::cout << "[5] vector has reserve() = "
            << has_reserve<std::vector<int>>::value << '\n';

  // [6] multiple requirements are combined in one void_t
  std::cout << "[6] vector is iterable = "
            << is_iterable<std::vector<int>>::value << '\n';
}
