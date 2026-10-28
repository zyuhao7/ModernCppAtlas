#include <cstddef>
#include <iostream>
#include <type_traits>

template <typename T>
bool empty(T* ptr) {
  return ptr == nullptr;
}

void consume(int*) { std::cout << "pointer\n"; }
void consume(std::nullptr_t) { std::cout << "null\n"; }

int main() {
  int* ptr = nullptr;
  static_assert(std::is_same<decltype(nullptr), std::nullptr_t>::value,
                "nullptr has its own type");

  std::cout << empty(ptr) << ' ';
  consume(nullptr);
}
