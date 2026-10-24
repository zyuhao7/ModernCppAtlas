#include <concepts>
#include <iostream>

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

int main() {
  std::cout << advance_one(41) << '\n';
}
