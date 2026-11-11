#include <cstdint>
#include <iostream>
#include <type_traits>

// 场景 1 与 2：先给出不含枚举值的声明，之后再定义。
enum class Mode : std::uint8_t;

enum class Mode : std::uint8_t { fast, safe };

// 场景 3：无作用域枚举也能前置声明，但必须写底层类型。
enum Color : unsigned char;

enum Color : unsigned char { red = 1, green = 2, blue = 4 };

// 场景 4：声明出现在函数签名与类成员里，此时还看不到枚举值。
enum class Level : short;

const char* level_name(Level level);

struct Settings {
  Level level;
};

enum class Level : short { low = -1, normal = 0, high = 1 };

const char* level_name(Level level) {
  switch (level) {
    case Level::low:
      return "low";
    case Level::normal:
      return "normal";
    case Level::high:
      return "high";
  }
  return "unknown";
}

Mode pick(bool fast) { return fast ? Mode::fast : Mode::safe; }

int main() {
  // 场景 1：声明之后即可在函数签名与返回值中使用枚举类型。
  std::cout << "[1] pick(true) = "
            << (pick(true) == Mode::fast ? "fast" : "safe")
            << ", pick(false) = "
            << (pick(false) == Mode::safe ? "safe" : "fast") << '\n';

  // 场景 2：底层类型固定后，声明处就能确定大小。
  std::cout << "[2] sizeof(Mode) = " << sizeof(Mode)
            << ", static_cast<Mode>(1) == safe = "
            << (static_cast<Mode>(1) == Mode::safe) << '\n';

  // 场景 3：无作用域枚举的声明与定义必须写相同的底层类型。
  std::cout << "[3] sizeof(Color) = " << sizeof(Color)
            << ", green = " << static_cast<int>(green) << '\n';

  // 场景 4：前置声明足以让函数参数与结构体成员通过编译。
  Settings settings{Level::high};
  std::cout << "[4] settings level = " << level_name(settings.level)
            << ", sizeof(Level) = " << sizeof(Level) << '\n';

  // 场景 5：声明里写的底层类型可以用类型萃取验证。
  std::cout
      << "[5] underlying type is uint8_t = "
      << std::is_same<std::underlying_type<Mode>::type, std::uint8_t>::value
      << '\n';
}
