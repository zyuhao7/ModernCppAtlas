#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>

struct Kilometers {
  double value;
};

struct Json {
  std::string text;
};

Kilometers operator""_km(unsigned long long value) {
  return Kilometers{static_cast<double>(value)};
}

Kilometers operator""_km(long double value) {
  return Kilometers{static_cast<double>(value)};
}

std::size_t operator""_length(const char* /*text*/, std::size_t size) {
  return size;
}

Json operator""_json(const char* text, std::size_t size) {
  return Json{std::string(text, size)};
}

namespace atlas {
std::string operator""_quoted(const char* text, std::size_t size) {
  return std::string("\"") + std::string(text, size) + "\"";
}
}  // namespace atlas

using atlas::operator""_quoted;

int main() {
  // 场景 1：整数与浮点后缀分别命中不同参数类型的重载。
  const Kilometers whole = 12_km;
  const Kilometers fractional = 12.5_km;
  std::cout << "[1] 12_km = " << whole.value
            << ", 12.5_km = " << fractional.value << ", type is Kilometers = "
            << std::is_same<decltype(12.5_km), Kilometers>::value << '\n';

  // 场景 2：字符串字面量重载拿到指针与长度，不需要 strlen。
  std::cout << "[2] \"atlas\"_length = " << "atlas"_length << '\n';

  // 场景 3：原始字符串加后缀，把 JSON 文本直接变成一个类型。
  const Json payload = R"({"ok":true})"_json;
  std::cout << "[3] json = " << payload.text << '\n';

  // 场景 4：字面量运算符可以声明在命名空间里，再由 using 引入。
  std::cout << "[4] quoted = " << "atlas"_quoted << '\n';
}
