#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>

using Value = std::variant<std::int64_t, std::string>;

Value parse(std::string_view text) {
  bool digits = !text.empty();
  for (char ch : text) {
    digits = digits && ch >= '0' && ch <= '9';
  }
  if (digits) {
    return static_cast<std::int64_t>(std::stoll(std::string(text)));
  }
  return std::string(text);
}

template <class... Fs>
struct Overloaded : Fs... {
  using Fs::operator()...;
};
template <class... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>;

void print(const Value& value) {
  std::visit(Overloaded{
                 [](std::int64_t number) { std::cout << "number " << number; },
                 [](const std::string& text) { std::cout << "text " << text; },
             },
             value);
}

int main() {
  const Value a = parse("42");
  const Value b = parse("atlas");
  std::cout << "[1] 42 -> ";
  print(a);
  std::cout << ", atlas -> ";
  print(b);
  std::cout << '\n';

  // get_if 不抛异常，未命中时返回空指针。
  const auto* number = std::get_if<std::int64_t>(&a);
  const auto* text = std::get_if<std::string>(&a);
  std::cout << "[2] get_if int = "
            << (number ? std::to_string(*number) : "null")
            << ", get_if string = " << (text ? *text : "null") << '\n';

  std::cout << "[3] index a = " << a.index() << ", index b = " << b.index()
            << ", holds string = " << std::holds_alternative<std::string>(b)
            << '\n';

  try {
    std::cout << std::get<std::string>(a) << '\n';
  } catch (const std::bad_variant_access&) {
    std::cout << "[4] wrong type -> bad_variant_access\n";
  }

  // monostate 让 variant 可以默认构造出“空状态”。
  std::variant<std::monostate, int, std::string> slot;
  std::cout << "[5] default index = " << slot.index()
            << ", monostate = " << std::holds_alternative<std::monostate>(slot)
            << '\n';
  slot = 7;
  std::cout << "[5] after assign index = " << slot.index()
            << ", value = " << std::get<int>(slot) << '\n';

  // P0608 之后 const char* 不再转换为 bool，而是选择 std::string；
  // 需要 bool 备选时用 in_place_type 明确指定。
  std::variant<std::string, bool> flag = "hello";
  std::variant<std::string, bool> enabled{std::in_place_type<bool>, true};
  std::cout << "[6] const char* -> string = "
            << std::holds_alternative<std::string>(flag)
            << ", value = " << std::get<std::string>(flag)
            << ", in_place bool = " << std::holds_alternative<bool>(enabled)
            << '\n';
}
