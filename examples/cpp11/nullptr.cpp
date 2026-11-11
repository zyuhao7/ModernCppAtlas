#include <cstddef>
#include <iostream>
#include <type_traits>

template <typename T>
bool empty(T* ptr) {
  return ptr == nullptr;
}

void consume(int*) { std::cout << "pointer\n"; }
void consume(std::nullptr_t) { std::cout << "null\n"; }

template <typename T>
const char* kind(T) {
  return std::is_same<T, std::nullptr_t>::value ? "nullptr_t" : "other";
}

int main() {
  // 场景 1：nullptr 有独立类型 std::nullptr_t，不是整数零。
  // 静态断言在编译期验证类型，empty 则演示与指针比较。
  static_assert(std::is_same<decltype(nullptr), std::nullptr_t>::value,
                "nullptr has its own type");
  int* ptr = nullptr;
  std::cout << "[1] empty = " << std::boolalpha << empty(ptr) << '\n';

  // 场景 2：重载决议中 nullptr 精确匹配 std::nullptr_t，指针匹配 int*。
  // 注意 consume(0) 无法编译：整数 0 同时是空指针常量，两个重载都可行。
  // 但在初始化指针时，整数 0 仍会被接受为空指针常量。
  std::cout << "[2] overloads:\n";
  consume(nullptr);
  consume(static_cast<int*>(nullptr));
  int* zero = 0;
  std::cout << "zero by literal = " << (empty(zero) ? "null" : "set") << '\n';

  // 场景 3：模板推导 nullptr 得到 std::nullptr_t，不会退化成 int。
  std::cout << "[3] deduced type = " << kind(nullptr) << '\n';

  // 场景 4：nullptr 可用于布尔上下文，!ptr 表示指针为空。
  int* none = nullptr;
  std::cout << "[4] bool context = " << (!none ? "empty" : "set") << '\n';
}
