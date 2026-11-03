#include <array>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>

template <typename T, T... Values>
constexpr std::array<T, sizeof...(Values)> to_array(
    std::integer_sequence<T, Values...>) {
  return {{Values...}};
}

template <typename Tuple, typename F, std::size_t... Is>
void for_each_impl(const Tuple& tuple, F&& func, std::index_sequence<Is...>) {
  int unused[] = {0, ((void)func(std::get<Is>(tuple)), 0)...};
  (void)unused;
}

template <typename Tuple, typename F>
void for_each_tuple(const Tuple& tuple, F&& func) {
  for_each_impl(tuple, func,
                std::make_index_sequence<std::tuple_size<Tuple>::value>{});
}

int main() {
  constexpr auto evens = to_array(std::integer_sequence<int, 0, 2, 4, 6>{});
  static_assert(evens.size() == 4, "four values");
  static_assert(evens[3] == 6, "last value");

  int sum = 0;
  for (int value : evens) {
    sum += value;
  }
  std::cout << evens.size() << ' ' << sum << '\n';

  const auto record = std::make_tuple(1, 2.5, std::string("atlas"));
  for_each_tuple(record, [](const auto& value) { std::cout << value << ' '; });
  std::cout << '\n';

  std::cout << std::tuple_size<decltype(record)>::value << '\n';
}
