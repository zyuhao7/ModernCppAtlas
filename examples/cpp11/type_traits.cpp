#include <iostream>
#include <type_traits>

template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type twice(T value) {
  return value + value;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type twice(
    T value) {
  return value * 2;
}

template <typename T>
struct is_pointer_to_const : std::false_type {};

template <typename T>
struct is_pointer_to_const<const T*> : std::true_type {};

int main() {
  std::cout << twice(21) << ' ' << twice(1.5) << '\n';

  static_assert(std::is_same<int, typename std::decay<const int&>::type>::value,
                "decay");
  static_assert(
      std::is_same<int, typename std::remove_reference<int&>::type>::value,
      "remove reference");

  std::cout << std::is_same<decltype(twice(1)), int>::value << ' '
            << is_pointer_to_const<const int*>::value << ' '
            << is_pointer_to_const<int*>::value << '\n';

  using chosen = std::conditional<sizeof(void*) == 8, long long, int>::type;
  std::cout << std::is_integral<chosen>::value << ' '
            << std::is_convertible<int, double>::value << '\n';
}
