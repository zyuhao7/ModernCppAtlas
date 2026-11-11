#include <cstdint>
#include <iostream>

// 底层类型固定为 unsigned char，存储宽度就是 1 字节。
enum class Color : unsigned char { red = 1, green, blue };
// 另一个枚举用 uint16_t，二者是不同类型，不能直接比较。
enum class Status : std::uint16_t { ok = 200, not_found = 404 };
enum class Mode { fast, safe };

const char* name(Color color) {
  switch (color) {
    case Color::red:
      return "red";
    case Color::green:
      return "green";
    case Color::blue:
      return "blue";
  }
  return "unknown";
}

int main() {
  // 场景 1：枚举值必须带类型名访问，转成整数必须显式 static_cast。
  // switch 只接受同一枚举类型的分支，拼错名字会编译失败。
  std::cout << "[1] color = " << static_cast<int>(Color::green)
            << ", name = " << name(Color::green)
            << ", status = " << static_cast<int>(Status::not_found) << '\n';

  // 场景 2：底层类型决定对象大小。Color 占 1 字节，Status 占 2 字节，
  // 与普通 enum 默认的 int 宽度不同。
  std::cout << "[2] sizeof(Color) = " << sizeof(Color)
            << ", sizeof(Status) = " << sizeof(Status) << '\n';

  // 场景 3：枚举值可以显式转换到范围外的数字，但转换结果不保证是
  // 合法枚举值；name 对未知值返回 "unknown"。
  Color next = static_cast<Color>(static_cast<unsigned char>(Color::blue) + 1);
  std::cout << "[3] blue + 1 = " << static_cast<int>(next)
            << ", name = " << name(next) << '\n';

  // 场景 4：同一枚举类型可以比较；Color::red == Mode::fast 会编译失败，
  // 因为作用域枚举不会隐式转换成整数或其他枚举。
  std::cout << "[4] same type compare = " << std::boolalpha
            << (Color::green == Color::green) << '\n';
}
