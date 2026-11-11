#include <iostream>
#include <string>
#include <type_traits>

class Buffer {
 public:
  explicit operator bool() const { return size_ != 0; }

  void push(int value) {
    if (value != 0) {
      ++size_;
    }
  }

 private:
  int size_ = 0;
};

class Money {
 public:
  explicit Money(int cents) : cents_(cents) {}

  explicit operator double() const { return cents_ / 100.0; }

 private:
  int cents_;
};

class Path {
 public:
  explicit Path(const char* value) : value_(value) {}

  explicit operator std::string() const { return value_; }

 private:
  std::string value_;
};

int main() {
  Buffer empty;
  Buffer full;
  full.push(7);

  // 场景 1：条件运算符的第一个操作数是上下文转换位置。
  std::cout << "[1] empty -> " << (empty ? "true" : "false") << ", full -> "
            << (full ? "true" : "false") << '\n';

  // 场景 2：!、&&、|| 的条件同样属于上下文转换。
  std::cout << "[2] !empty = " << !empty
            << ", full && true = " << (full && true) << '\n';

  // 场景 3：隐式转换被禁止，直接初始化与 static_cast 仍可用。
  std::cout << "[3] is_convertible<Buffer, bool> = "
            << std::is_convertible<Buffer, bool>::value
            << ", is_constructible<bool, Buffer> = "
            << std::is_constructible<bool, Buffer>::value
            << ", static_cast<bool>(full) = " << static_cast<bool>(full)
            << '\n';

  // 场景 4：explicit 可以修饰任意目标类型，不限于 bool。
  Money money(1250);
  std::cout << "[4] static_cast<double>(money) = " << static_cast<double>(money)
            << ", is_convertible<Money, double> = "
            << std::is_convertible<Money, double>::value << '\n';

  // 场景 5：有代价的字符串转换只在显式调用时发生。
  Path path("/tmp/atlas");
  std::string text = static_cast<std::string>(path);
  std::cout << "[5] text = " << text << ", is_convertible<Path, std::string> = "
            << std::is_convertible<Path, std::string>::value << '\n';
}
