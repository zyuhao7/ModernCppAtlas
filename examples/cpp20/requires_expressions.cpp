#include <concepts>
#include <cstddef>
#include <iostream>

template <typename T>
concept HasValue = requires(T value) { value.value; };

template <typename T>
concept Sized = requires(const T &value) {
  typename T::value_type;
  { value.size() } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept NoexceptSized = requires(const T &value) {
  { value.size() } noexcept -> std::convertible_to<std::size_t>;
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

struct Entry {
  using value_type = int;
  int value;
  std::size_t size() const { return 1; }
};

struct Empty {};

struct WrongSize {
  using value_type = int;
  void size() const {}
};

struct NoThrow {
  std::size_t size() const noexcept { return 1; }
};

struct Throwing {
  std::size_t size() const { return 1; }
};

int main() {
  std::cout << "[1] Sized<Range> = " << Sized<Range> << ", Sized<int> = "
            << Sized<int> << '\n';
  std::cout << "[2] length(Range{}) = " << length(Range{})
            << ", length(42) = " << length(42) << '\n';
  std::cout << "[3] HasValue<Entry> = "
            << HasValue<Entry> << ", HasValue<Empty> = "
            << HasValue<Empty> << '\n';
  std::cout << "[4] Sized<WrongSize> = " << Sized<WrongSize> << '\n';

  static_assert(Sized<Range>, "Range satisfies Sized");
  std::cout << "[5] static_assert passed = 1\n";

  std::cout << "[6] NoexceptSized<NoThrow> = "
            << NoexceptSized<NoThrow> << ", NoexceptSized<Throwing> = "
            << NoexceptSized<Throwing> << '\n';
}
