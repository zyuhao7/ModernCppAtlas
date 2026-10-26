#include <iostream>
#include <source_location>
#include <string_view>

void trace(std::string_view tag,
           const std::source_location &loc = std::source_location::current()) {
  std::cout << tag << ' ' << loc.line() << '\n';
}

void wrapper(std::string_view tag,
             const std::source_location &loc = std::source_location::current()) {
  trace(tag, loc);
}

int main() {
  trace("first");
  wrapper("second");

  auto loc = std::source_location::current();
  std::cout << (loc.line() > 0) << ' '
            << (std::string_view(loc.function_name()).find("main") !=
                std::string_view::npos)
            << '\n';
}
