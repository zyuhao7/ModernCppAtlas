#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <version>

#if defined(__cpp_lib_print)
#include <print>
#endif

// 两条输出路径产生完全相同的文本，快照不依赖编译器是否提供 <print>。
void emit(const std::string& text) {
#if defined(__cpp_lib_print)
  std::println("{}", text);
#else
  std::cout << text << '\n';
#endif
}

struct Point {
  int x;
  int y;
};

#if defined(__cpp_lib_format)
template <>
struct std::formatter<Point> {
  constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

  auto format(const Point& point, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "({}, {})", point.x, point.y);
  }
};
#endif

int main() {
  emit(std::format("[1] hello, {}", "atlas"));

  emit(std::format("[2] {} + {} = {}", 2, 3, 2 + 3));

  // 对齐、填充与零填充都写在格式串里，输出宽度固定。
  emit(std::format("[3] [{:>8.2f}] [{:<5}] [{:04d}]", 3.14159, "ab", 42));

  const Point point{2, 5};
  emit(std::format("[4] point = {}", point));

  const std::string_view name = "atlas";
  const int count = 3;
  emit(std::format("[5] {0}-{1}-{0}", name, count));

  // 转义花括号得到字面量，否则会被当作占位符。
  emit(std::format("[6] literal {{}} braces"));
}
