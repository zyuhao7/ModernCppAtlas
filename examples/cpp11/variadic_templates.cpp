#include <cstddef>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

std::size_t count_args() { return 0; }

template <typename T, typename... Rest>
std::size_t count_args(const T&, const Rest&... rest) {
  return 1 + count_args(rest...);
}

void print_rest() {}

template <typename T, typename... Rest>
void print_rest(const T& first, const Rest&... rest) {
  std::cout << ' ' << first;
  print_rest(rest...);
}

template <typename T, typename... Rest>
void print_all(const T& first, const Rest&... rest) {
  std::cout << first;
  print_rest(rest...);
  std::cout << '\n';
}

template <typename... Args>
std::size_t pack_size(const Args&...) {
  return sizeof...(Args);
}

int sum_all() { return 0; }

template <typename... Rest>
int sum_all(int first, Rest... rest) {
  return first + sum_all(rest...);
}

template <typename... Args>
std::vector<std::string> pack_strings(Args&&... args) {
  return std::vector<std::string>{std::forward<Args>(args)...};
}

int main() {
  // 场景 1：递归展开参数包，每层取一个实参，空包匹配无参重载终止。
  std::cout << "[1] args:";
  print_all(1, "two", 3.5);
  std::cout << "[1] count = " << count_args(1, "two", 3.5) << '\n';

  // 场景 2：sizeof... 在编译期直接给出包内元素个数，不需要递归。
  std::cout << "[2] pack_size(1,2,3,4) = " << pack_size(1, 2, 3, 4) << '\n';

  // 场景 3：同类型实参可以递归累加，每个实参生成一层实例化。
  std::cout << "[3] sum = " << sum_all(1, 2, 3, 4) << '\n';

  // 场景 4：参数包展开成初始化列表，std::forward 保留每个实参的值类别。
  std::vector<std::string> items = pack_strings("a", std::string("b"), "c");
  std::cout << "[4] size = " << items.size() << ", joined =";
  for (const auto& item : items) std::cout << ' ' << item;
  std::cout << '\n';

  // 场景 5：异构类型装进 tuple，参数包在类模板里描述成员类型。
  std::tuple<int, std::string, double> record{1, "two", 3.5};
  std::cout << "[5] tuple size = " << std::tuple_size<decltype(record)>::value
            << ", field 1 = " << std::get<1>(record) << '\n';
}
