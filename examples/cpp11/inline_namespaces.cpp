#include <iostream>

namespace atlas {

// 内联命名空间：符号带 ABI 版本，用户仍写 atlas::version。
inline namespace v2 {

constexpr int version = 2;

struct Widget {
  int size;
};

void describe(Widget widget) {
  std::cout << "widget " << widget.size << " bytes\n";
}

template <class T>
struct Traits {
  enum : int { id = 0 };
};

}  // namespace v2
}  // namespace atlas

// 显式特化必须写出 ABI 命名空间，不能只写 atlas::Traits。
template <>
struct atlas::v2::Traits<int> {
  enum : int { id = 7 };
};

int main() {
  std::cout << "[1] atlas::version = " << atlas::version << '\n';
  std::cout << "[2] atlas::v2::version = " << atlas::v2::version << '\n';
  std::cout << "[3] sizeof(atlas::Widget) = " << sizeof(atlas::Widget) << '\n';
  std::cout << "[4] atlas::Traits<int>::id = " << atlas::Traits<int>::id
            << '\n';

  // 参数类型位于内联命名空间，ADL 会找到 atlas::v2::describe。
  std::cout << "[5] ADL describe: ";
  describe(atlas::Widget{4});
}
