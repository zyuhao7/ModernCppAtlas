#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

template <typename T, typename U>
auto add(T a, U b) -> decltype(a + b) {
  return a + b;
}

template <typename Container>
auto element(Container& container, std::size_t index)
    -> decltype(container[index]) {
  return container[index];
}

auto half(int value) noexcept -> int { return value / 2; }

int main() {
  // 场景 1：返回类型由参数的加法表达式决定。
  std::cout << "[1] add(2, 3) = " << add(2, 3)
            << ", add(1.5, 2.0) = " << add(1.5, 2.0) << ", int result = "
            << std::is_same<decltype(add(2, 3)), int>::value
            << ", double result = "
            << std::is_same<decltype(add(1.5, 2.0)), double>::value << '\n';

  // 场景 2：尾置返回同样适用于返回用户定义类型。
  std::string text = add(std::string("atlas"), "!");
  std::cout
      << "[2] text = " << text << ", type is string = "
      << std::is_same<decltype(add(std::string("a"), "b")), std::string>::value
      << '\n';

  // 场景 3：decltype 保留引用，返回值可以直接修改容器元素。
  std::vector<int> values{1, 2, 3};
  auto& slot = element(values, 1);
  slot = 20;
  std::cout << "[3] values[1] = " << values[1] << ", returns int& = "
            << std::is_same<decltype(element(values, 1)), int&>::value << '\n';

  // 场景 4：noexcept 写在尾置返回类型之后。
  std::cout << "[4] half(9) = " << half(9)
            << ", noexcept = " << noexcept(half(9)) << '\n';

  // 场景 5：混合类型表达式的推导结果可以直接验证。
  std::cout << "[5] add(1, 2.5) = " << add(1, 2.5) << ", type is double = "
            << std::is_same<decltype(add(1, 2.5)), double>::value << '\n';
}
