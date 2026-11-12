#include <cerrno>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>

enum class ParseError { empty = 1, overflow = 2 };

class ParseCategory : public std::error_category {
 public:
  const char* name() const noexcept override { return "parse"; }

  std::string message(int value) const override {
    switch (static_cast<ParseError>(value)) {
      case ParseError::empty:
        return "empty input";
      case ParseError::overflow:
        return "value overflow";
    }
    return "unknown parse error";
  }
};

const std::error_category& parse_category() {
  static const ParseCategory instance;
  return instance;
}

std::error_code make_error_code(ParseError error) {
  return {static_cast<int>(error), parse_category()};
}

std::error_code parse_positive(int value) {
  if (value == 0) return make_error_code(ParseError::empty);
  if (value < 0) return make_error_code(ParseError::overflow);
  return {};
}

int main() {
  // 默认构造的 error_code 表示成功，false 即为“没有错误”。
  const std::error_code ok;
  const std::error_code missing =
      std::make_error_code(std::errc::no_such_file_or_directory);
  std::cout << "[1] ok = " << ok.value() << ", missing = " << missing.value()
            << ", message = " << missing.message()
            << ", category = " << missing.category().name() << '\n';

  // 把 C 函数的 errno 转成 error_code，错误不经过异常。
  errno = 0;
  std::ifstream input("/definitely/not/here");
  const std::error_code open_error =
      errno == 0 ? std::error_code{}
                 : std::error_code{errno, std::generic_category()};
  std::cout << "[2] opened = " << static_cast<bool>(input)
            << ", failed = " << static_cast<bool>(open_error)
            << ", message = " << open_error.message() << '\n';

  // std::system_error 携带 error_code，适合必须抛异常的场景。
  try {
    throw std::system_error(missing, "open config");
  } catch (const std::system_error& error) {
    std::cout << "[3] code message = " << error.code().message()
              << ", matches = "
              << (error.code() == std::errc::no_such_file_or_directory) << '\n';
  }

  // 自定义类别让库可以定义自己的错误空间。
  const std::error_code empty = make_error_code(ParseError::empty);
  const std::error_code overflow = make_error_code(ParseError::overflow);
  std::cout << "[4] empty = " << empty.category().name() << ": "
            << empty.message() << ", overflow = " << overflow.message() << '\n';

  // 数值相同但类别不同，error_code 并不相等。
  const std::error_code generic{7, std::generic_category()};
  const std::error_code system{7, std::system_category()};
  std::cout << "[5] same value = " << (generic.value() == system.value())
            << ", equal = " << (generic == system) << '\n';

  // 成功路径返回空 error_code，调用方只检查布尔值即可。
  std::cout << "[6] parse(5) ok = " << !parse_positive(5)
            << ", parse(-1) message = " << parse_positive(-1).message() << '\n';
}
