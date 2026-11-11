#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>

std::tuple<std::string, int, bool> parse(const std::string& name, int score) {
  return std::make_tuple(name, score, score >= 60);
}

int main() {
  // 场景 1：make_tuple 推导元素类型，get<I> 按下标访问。
  auto result = parse("alice", 82);
  std::cout << "[1] " << std::get<0>(result) << ' ' << std::get<1>(result)
            << ' ' << (std::get<2>(result) ? "pass" : "fail") << '\n';

  // 场景 2：tuple_size 与 tuple_element 在编译期查询长度和类型。
  static_assert(std::tuple_size<decltype(result)>::value == 3, "three fields");
  static_assert(
      std::is_same<std::tuple_element<1, decltype(result)>::type, int>::value,
      "score is int");
  std::cout
      << "[2] size = " << std::tuple_size<decltype(result)>::value
      << ", element 1 is int = " << std::boolalpha
      << std::is_same<std::tuple_element<1, decltype(result)>::type, int>::value
      << '\n';

  // 场景 3：tie 把已有变量绑成引用元组，ignore 跳过不需要的位置。
  int score = 0;
  bool passed = false;
  std::tie(std::ignore, score, passed) = parse("bob", 45);
  std::cout << "[3] score = " << score
            << ", result = " << (passed ? "pass" : "fail") << '\n';

  // 场景 4：tuple_cat 把两个 tuple 拼成一个更长的 tuple。
  auto extended = std::tuple_cat(result, std::make_tuple("extra"));
  std::cout << "[4] last = " << std::get<3>(extended)
            << ", size = " << std::tuple_size<decltype(extended)>::value
            << '\n';

  // 场景 5：元素都支持比较时，tuple 按字典序逐元素比较，
  // 先比较第一个不同的元素。这里 1 相同，再比较 "b" 与 "c"。
  auto a = std::make_tuple(1, std::string("b"));
  auto b = std::make_tuple(1, std::string("c"));
  std::cout << "[5] a < b = " << (a < b) << '\n';
}
