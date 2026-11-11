#include <iostream>
#include <source_location>
#include <string_view>

bool has_text(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

void log(std::string_view tag,
         const std::source_location &loc = std::source_location::current()) {
  std::cout << "[1] tag = " << tag
            << ", at main = " << has_text(loc.function_name(), "main") << '\n';
}

void wrapper(std::string_view tag, const std::source_location &loc =
                                       std::source_location::current()) {
  const auto here = std::source_location::current();
  std::cout << "[2] tag = " << tag
            << ", forwarded main = " << has_text(loc.function_name(), "main")
            << ", body wrapper = " << has_text(here.function_name(), "wrapper")
            << '\n';
}

int main() {
  log("first");
  wrapper("second");

  const auto loc = std::source_location::current();
  std::cout << "[3] file non-empty = "
            << (loc.file_name() != nullptr && loc.file_name()[0] != '\0')
            << ", column positive = " << (loc.column() > 0) << '\n';
  std::cout << "[4] line positive = " << (loc.line() > 0)
            << ", function main = " << has_text(loc.function_name(), "main")
            << '\n';

  const auto copied = loc;
  std::cout << "[5] copied file matches = "
            << (std::string_view(copied.file_name()) ==
                std::string_view(loc.file_name()))
            << ", copied column matches = " << (copied.column() == loc.column())
            << '\n';
}
