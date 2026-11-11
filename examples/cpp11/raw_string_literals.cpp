#include <cstddef>
#include <iostream>
#include <string>

int main() {
  // 场景 1：反斜杠原样保留，与普通转义写法完全等价。
  std::string raw = R"(\d+\.\d+)";
  std::string escaped = "\\d+\\.\\d+";
  std::cout << "[1] raw = " << raw << ", equals escaped = " << (raw == escaped)
            << '\n';

  // 场景 2：JSON 里的双引号不需要转义。
  std::string json = R"({"name":"atlas","size":3})";
  std::cout << "[2] json = " << json << '\n';

  // 场景 3：自定义分隔符让内容可以包含 )"
  std::string custom = R"json({"text":")"})json";
  std::cout << "[3] custom = " << custom << '\n';

  // 场景 4：源码中的换行直接成为字符串内容。
  std::string lines = R"(line one
line two)";
  std::size_t newlines = 0;
  for (char ch : lines) {
    if (ch == '\n') ++newlines;
  }
  std::cout << "[4] newlines = " << newlines << ", bytes = " << lines.size()
            << '\n';

  // 场景 5：原始字符串与普通字符串可以拼接。
  std::string built = R"({"id":)" + std::to_string(42) + R"(})";
  std::cout << "[5] built = " << built << '\n';
}
