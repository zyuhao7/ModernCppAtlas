#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
struct Box {
  T value;
};

template <typename T>
Box(T) -> Box<T>;

template <typename T>
struct PtrBox {
  T* ptr;
};

template <typename T>
PtrBox(T*) -> PtrBox<T>;

int main() {
  std::pair p{1, 2.5};
  static_assert(std::is_same<decltype(p), std::pair<int, double>>::value,
                "pair CTAD");
  std::cout << "[1] pair = " << p.first << ' ' << p.second << '\n';

  std::vector v{1, 2, 3};
  static_assert(std::is_same<decltype(v), std::vector<int>>::value,
                "vector CTAD");
  std::cout << "[2] vector size = " << v.size() << ", values = " << v[0] << ' '
            << v[1] << ' ' << v[2] << '\n';

  Box box{42};
  static_assert(std::is_same<decltype(box), Box<int>>::value, "Box guide");
  std::cout << "[3] box = " << box.value << ", type is Box<int> = 1\n";

  int number = 42;
  PtrBox pbox{&number};
  static_assert(std::is_same<decltype(pbox), PtrBox<int>>::value,
                "PtrBox guide");
  std::cout << "[4] ptrbox = " << *pbox.ptr << ", type is PtrBox<int> = 1\n";

  std::vector names{"a", "bb"};
  static_assert(std::is_same<decltype(names), std::vector<const char*>>::value,
                "initializer_list deduces const char*");
  std::vector<std::string> strings{"a", "bb"};
  std::cout << "[5] names deduce const char* = 1, explicit strings = "
            << strings[0] << ' ' << strings[1] << '\n';

  std::tuple record{1, 2.5, std::string("atlas")};
  std::cout << "[6] tuple size = " << std::tuple_size<decltype(record)>::value
            << '\n';
}
