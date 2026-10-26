#include <concepts>
#include <cstddef>
#include <iostream>

template <typename T>
concept Sized = requires(const T &value) {
  typename T::value_type;
  { value.size() } -> std::convertible_to<std::size_t>;
};

template <Sized T>
std::size_t length(const T &value) {
  return value.size();
}

template <typename T>
std::size_t length(const T &) {
  return 0;
}

struct Range {
  using value_type = int;
  std::size_t size() const { return 3; }
};

int main() {
  std::cout << Sized<Range> << ' ' << Sized<int> << ' ' << length(Range{})
            << ' ' << length(42) << '\n';
}
