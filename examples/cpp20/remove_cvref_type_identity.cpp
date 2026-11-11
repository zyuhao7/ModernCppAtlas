#include <iostream>
#include <type_traits>

template <typename T>
struct Holder {
  using value_type = std::remove_cvref_t<T>;
  T value;
};
template <typename T>
Holder(T &&) -> Holder<T>;

template <typename T>
void select(T value, std::type_identity_t<T> same) {
  std::cout << value + same << '\n';
}

int main() {
  static_assert(std::is_same_v<std::remove_cvref_t<const int &>, int>);
  static_assert(std::is_same_v<std::remove_cvref_t<const int (&)[3]>, int[3]>);

  std::cout
      << "[1] const int& -> int = "
      << std::is_same_v<std::remove_cvref_t<const int &>,
                        int> << ", const int(&)[3] -> int[3] = "
      << std::is_same_v<std::remove_cvref_t<const int (&)[3]>, int[3]> << '\n';

  std::cout
      << "[2] remove_cvref keeps array = "
      << std::is_same_v<std::remove_cvref_t<const int (&)[3]>,
                        int[3]> << ", decay makes pointer = "
      << std::is_same_v<std::decay_t<const int (&)[3]>, const int *> << '\n';

  int x = 5;
  Holder held{x};
  static_assert(std::is_same_v<decltype(held.value), int &>);
  static_assert(std::is_same_v<Holder<int &>::value_type, int>);
  held.value = 7;
  std::cout
      << "[3] held type is int& = "
      << std::is_same_v<decltype(held.value), int &> << ", value_type is int = "
      << std::is_same_v<Holder<int &>::value_type, int> << ", x = " << x
      << '\n';

  std::type_identity_t<int> three = 3;
  std::cout << "[4] select(held.value, 3) = ";
  select(held.value, three);

  const int y = 9;
  Holder constant{y};
  static_assert(std::is_same_v<decltype(constant.value), const int &>);
  std::cout << "[5] const holder = " << constant.value
            << ", value_type is int = "
            << std::is_same_v<decltype(constant)::value_type, int> << '\n';
}
