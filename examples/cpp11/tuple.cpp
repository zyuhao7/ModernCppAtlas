#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>

std::tuple<std::string, int, bool> parse(const std::string& name, int score) {
  return std::make_tuple(name, score, score >= 60);
}

int main() {
  auto result = parse("alice", 82);
  std::cout << std::get<0>(result) << ' ' << std::get<1>(result) << ' '
            << (std::get<2>(result) ? "pass" : "fail") << '\n';

  static_assert(std::tuple_size<decltype(result)>::value == 3, "three fields");
  static_assert(
      std::is_same<std::tuple_element<1, decltype(result)>::type, int>::value,
      "score is int");

  int score = 0;
  bool passed = false;
  std::tie(std::ignore, score, passed) = parse("bob", 45);
  std::cout << score << ' ' << (passed ? "pass" : "fail") << '\n';

  auto extended = std::tuple_cat(result, std::make_tuple("extra"));
  std::cout << std::get<3>(extended) << ' '
            << std::tuple_size<decltype(extended)>::value << '\n';

  auto a = std::make_tuple(1, std::string("b"));
  auto b = std::make_tuple(1, std::string("c"));
  std::cout << (a < b ? "less" : "greater") << '\n';
}
