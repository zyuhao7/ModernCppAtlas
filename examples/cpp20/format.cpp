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
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(const Point& p, format_context& ctx) const {
    return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
  }
};
}  // namespace std

int main() {
  std::string basic = std::format("{:.2f} {} {}", 3.14159, "atlas", 42);
  std::cout << "[1] " << basic << '\n';

  std::string aligned =
      std::format("[{:>8.2f}|{:<5}|{:04d}]", 3.14159, "ab", 42);
  std::cout << "[2] " << aligned << '\n';

  std::string point = std::format("{}", Point{2, 5});
  std::cout << "[3] point = " << point << '\n';

  std::string out;
  std::format_to(std::back_inserter(out), "{} -> {:.2f} {}", Point{2, 5},
                 3.14159, "atlas");
  std::cout << "[4] built = " << out << '\n';

  std::string reordered = std::format("{0}-{1}-{0}", "atlas", 42);
  std::cout << "[5] reordered = " << reordered << '\n';
}
