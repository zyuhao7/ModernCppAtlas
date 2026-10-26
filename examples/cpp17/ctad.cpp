#include <iostream>
#include <utility>
#include <vector>

template <typename T> struct Box {
  T value;
};

template <typename T> Box(T) -> Box<T>;

template <typename T> struct PtrBox {
  T *ptr;
};

template <typename T> PtrBox(T *) -> PtrBox<T>;

int main() {
  Box box{42};
  PtrBox pbox{&box};
  std::pair p{1, 2.5};
  std::vector v{1, 2, 3};

  std::cout << box.value << ' ' << pbox.ptr->value << ' ' << p.first << ' '
            << v.size() << '\n';
}
