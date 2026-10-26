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

  int x = 5;
  Holder held{x};
  static_assert(std::is_same_v<decltype(held.value), int &>);
  static_assert(std::is_same_v<Holder<int &>::value_type, int>);

  held.value = 7;
  std::type_identity_t<int> three = 3;
  select(held.value, three);
  std::cout << x << '\n';
}
