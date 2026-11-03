#include <iostream>
#include <string>
#include <tuple>

std::tuple<std::string, int, bool> parse(const std::string& name, int score) {
  return std::make_tuple(name, score, score >= 60);
}

int main() {
  auto result = parse("alice", 82);
  std::cout << std::get<0>(result) << ' ' << std::get<1>(result) << ' '
            << (std::get<2>(result) ? "pass" : "fail") << '\n';

  int score = 0;
  bool passed = false;
  std::tie(std::ignore, score, passed) = parse("bob", 45);
  std::cout << score << ' ' << (passed ? "pass" : "fail") << '\n';
}
