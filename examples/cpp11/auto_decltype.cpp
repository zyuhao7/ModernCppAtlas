#include <cstddef>
#include <iostream>
#include <type_traits>
#include <vector>

template <typename Container>
auto middle(const Container& values) -> decltype(values.begin()) {
  return values.begin() + static_cast<std::ptrdiff_t>(values.size() / 2);
}

int main() {
  std::vector<int> values{10, 20, 30, 40, 50};

  // 场景 1：auto 按值推导会丢掉引用与顶层 const；auto& 才保留它们。
  int x = 1;
  const int& ref = x;
  auto copied = ref;
  auto& bound = ref;
  static_assert(std::is_same<decltype(copied), int>::value,
                "auto strips const and reference");
  static_assert(std::is_same<decltype(bound), const int&>::value,
                "auto& keeps const and reference");
  copied = 2;
  std::cout << "[1] copied = " << copied << ", x = " << x << '\n';

  // 场景 2：decltype(x) 是变量声明类型 int；decltype((x)) 是带括号的
  // 左值表达式，得到 int&，alias 成为 x 的别名。
  decltype(x) plain = 5;
  decltype((x)) alias = x;
  static_assert(std::is_same<decltype(plain), int>::value,
                "decltype(x) is int");
  static_assert(std::is_same<decltype((x)), int&>::value,
                "decltype((x)) is int&");
  alias = 7;
  std::cout << "[2] plain = " << plain << ", x after alias = " << x << '\n';

  // 场景 3：C++11 没有返回类型推导，尾置返回类型用 decltype 表达
  // 依赖参数的迭代器类型。
  auto it = middle(values);
  std::cout << "[3] middle = " << *it << '\n';

  // 场景 4：decltype 精确复用 size() 的返回类型，避免手写 size_t。
  decltype(values.size()) count = values.size();
  std::cout << "[4] count = " << count << ", unsigned = " << std::boolalpha
            << std::is_unsigned<decltype(count)>::value << '\n';
}
