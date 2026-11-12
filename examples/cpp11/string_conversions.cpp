#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

int main() {
  // to_string 对整数精确转换，对浮点数使用固定的小数位。
  std::cout << "[1] " << std::to_string(42) << ' ' << std::to_string(-7L) << ' '
            << std::to_string(9000000000LL) << ' ' << std::to_string(3.5)
            << '\n';

  // stoi 解析前缀，pos 报告实际消费了多少个字符。
  std::size_t pos = 0;
  const int parsed = std::stoi("42abc", &pos);
  std::cout << "[2] parsed = " << parsed << ", consumed = " << pos << '\n';

  // 显式指定进制。
  const int hex = std::stoi("ff", nullptr, 16);
  std::cout << "[3] hex = " << hex << '\n';

  // 完全无法解析时抛出 invalid_argument。
  try {
    (void)std::stoi("abc");
  } catch (const std::invalid_argument&) {
    std::cout << "[4] invalid_argument caught\n";
  }

  // 超出目标类型范围时抛出 out_of_range。
  try {
    (void)std::stoi("99999999999999999999");
  } catch (const std::out_of_range&) {
    std::cout << "[5] out_of_range caught\n";
  }

  // stod 同样报告消费位置，尾部非数字内容被留在字符串里。
  std::size_t used = 0;
  const double ratio = std::stod("3.14159px", &used);
  std::cout << "[6] ratio = " << ratio << ", used = " << used << '\n';
}
