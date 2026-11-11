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
  if (value < low) {
    return low;
  }
  if (high < value) {
    return high;
  }
  return value;
}

template <std::ranges::input_range R>
  requires Numeric<std::ranges::range_value_t<R>>
auto sum(const R& range) {
  using T = std::ranges::range_value_t<R>;
  T total{};
  for (const auto& value : range) {
    total += value;
  }
  return total;
}

struct WithSize {
  int size() const { return 3; }
};

struct WithoutSize {};

template <typename T>
constexpr bool has_size = requires(const T& value) { value.size(); };

int main() {
  static_assert(Incrementable<int>);
  static_assert(Numeric<double>);
  static_assert(!Numeric<std::string>);

  std::cout << "[1] advance int = " << advance_one(41)
            << ", advance double = " << advance_one(3.5) << '\n';
  std::cout << "[2] clamp int = " << clamp_value(15, 0, 10)
            << ", clamp double = " << clamp_value(2.5, 0.0, 3.0) << '\n';

  const std::vector<int> values{1, 2, 3};
  const std::vector<double> fractions{0.5, 1.25};
  std::cout << "[3] sum ints = " << sum(values)
            << ", sum doubles = " << sum(fractions) << '\n';

  std::cout << "[4] Incrementable<int> = "
            << Incrementable<int> << ", Numeric<double> = "
            << Numeric<double> << ", Numeric<std::string> = "
            << Numeric<std::string> << '\n';

  std::cout << "[5] has_size = " << has_size<WithSize> << ", missing = "
            << has_size<WithoutSize> << '\n';
}
