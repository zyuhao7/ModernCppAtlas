#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>

using Value = std::variant<std::int64_t, std::string>;

Value parse(std::string_view text) {
  bool digits = !text.empty();
  for (char ch : text) digits = digits && ch >= '0' && ch <= '9';
  if (digits) return static_cast<std::int64_t>(std::stoll(std::string(text)));
  return std::string(text);
}

template <class... Fs>
struct Overloaded : Fs... {
  using Fs::operator()...;
};
template <class... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>;

void print(const Value &value) {
  std::visit(
      Overloaded{
          [](std::int64_t number) { std::cout << "number " << number << '\n'; },
          [](const std::string &text) { std::cout << "text " << text << '\n'; },
      },
      value);
}

int main() {
  Value a = parse("42");
  Value b = parse("atlas");
  Value c = 7;

  print(a);
  print(b);
  print(c);

  std::cout << std::get<std::int64_t>(a) << '\n';
  std::cout << (std::get_if<std::string>(&a) == nullptr) << '\n';
  std::cout << a.index() << ' ' << b.index() << '\n';

  std::cout << std::holds_alternative<std::string>(b) << '\n';

  try {
    std::cout << std::get<std::string>(a) << '\n';
  } catch (const std::bad_variant_access &) {
    std::cout << "bad_variant_access\n";
  }
}
