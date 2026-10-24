#include <concepts>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

template <typename T>
concept Incrementable = requires(T value) {
  { ++value } -> std::same_as<T&>;
  value++;
};

template <Incrementable T>
T advance_one(T value) {
  ++value;
  return value;
}

template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template <Numeric T>
T clamp_value(T value, T low, T high) {
  if (value < low) return low;
  if (high < value) return high;
  return value;
}

template <std::ranges::input_range R>
  requires Numeric<std::ranges::range_value_t<R>>
auto sum(const R& range) {
  using T = std::ranges::range_value_t<R>;
  T total{};
  for (const auto& value : range) total += value;
  return total;
}

int main() {
  std::cout << advance_one(41) << '\n';
  std::cout << clamp_value(15, 0, 10) << '\n';
  std::cout << clamp_value(2.5, 0.0, 3.0) << '\n';

  const std::vector<int> values{1, 2, 3};
  std::cout << sum(values) << '\n';

  static_assert(Incrementable<int>);
  static_assert(Numeric<double>);
  static_assert(!Numeric<std::string>);
}
