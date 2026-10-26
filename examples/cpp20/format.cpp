#include <format>
#include <iostream>
#include <iterator>
#include <string>

struct Point {
  int x;
  int y;
};

namespace std {
template <>
struct formatter<Point> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  auto format(const Point &p, format_context &ctx) const {
    return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
  }
};
}  // namespace std

int main() {
  std::string line = std::format("{:>8.2f}|{:<5}|{:04d}", 3.14159, "ab", 42);
  std::string out;
  std::format_to(std::back_inserter(out), "{} -> {}", Point{2, 5}, line);
  std::cout << out << '\n';
}
