#include <array>
#include <cstddef>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>

template <typename T, T... Values>
constexpr std::array<T, sizeof...(Values)> to_array(
    std::integer_sequence<T, Values...>) {
  return {{Values...}};
}

template <std::size_t... Is>
constexpr std::array<int, sizeof...(Is)> squares(std::index_sequence<Is...>) {
  return {{static_cast<int>(Is * Is)...}};
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

template <std::size_t... Is>
constexpr std::size_t count_indices(std::index_sequence<Is...>) {
  return sizeof...(Is);
}

template <typename T, std::size_t N>
void print_array(const std::array<T, N>& values) {
  for (std::size_t i = 0; i < N; ++i) {
    if (i != 0) {
      std::cout << ' ';
    }
    std::cout << values[i];
  }
  std::cout << '\n';
}

int main() {
  constexpr auto evens = to_array(std::integer_sequence<int, 0, 2, 4, 6>{});
  static_assert(evens.size() == 4, "four values");
  static_assert(evens[3] == 6, "last value");

  int sum = 0;
  for (int value : evens) {
    sum += value;
  }
  std::cout << "[1] values = ";
  print_array(evens);
  std::cout << "[1] size = " << evens.size() << ", sum = " << sum << '\n';

  constexpr auto table = squares(std::make_index_sequence<4>{});
  std::cout << "[2] squares = ";
  print_array(table);

  const auto record = std::make_tuple(1, 2.5, std::string("atlas"));
  std::cout << "[3] tuple = ";
  std::size_t seen = 0;
  for_each_tuple(record, [&seen](const auto& value) {
    if (seen++ != 0) {
      std::cout << ' ';
    }
    std::cout << value;
  });
  std::cout << '\n';

  std::cout << "[4] tuple size = " << std::tuple_size<decltype(record)>::value
            << ", index count = "
            << count_indices(
                   std::index_sequence_for<int, double, std::string>{})
            << '\n';

  constexpr auto letters =
      to_array(std::integer_sequence<char, 'a', 'b', 'c'>{});
  std::cout << "[5] letters = ";
  print_array(letters);
}
