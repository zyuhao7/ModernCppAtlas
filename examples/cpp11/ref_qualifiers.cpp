#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

class Message {
 public:
  std::string& text() & { return text_; }
  std::string text() && { return std::move(text_); }

 private:
  std::string text_ = "atlas";
};

class Probe {
 public:
  const char* kind() & { return "lvalue"; }
  const char* kind() const& { return "const lvalue"; }
  const char* kind() && { return "rvalue"; }
  const char* kind() const&& { return "const rvalue"; }
};

int main() {
  // 场景 1：左值调用返回引用，通过引用修改的就是成员本身。
  Message message;
  std::string& reference = message.text();
  reference += "!";
  std::cout << "[1] after mutation = " << message.text() << '\n';

  // 场景 2：临时对象命中 && 重载，成员被移动出来而不是复制。
  std::string moved = Message{}.text();
  std::cout << "[2] moved = " << moved << '\n';

  // 场景 3：四个重载按对象的值类别与 cv 组合分别命中。
  Probe probe;
  const Probe const_probe;
  std::cout << "[3] " << probe.kind() << ", " << const_probe.kind() << ", "
            << Probe{}.kind() << ", " << std::move(const_probe).kind() << '\n';

  // 场景 4：返回类型随值类别变化，可以直接用 decltype 验证。
  std::cout << "[4] lvalue yields string& = "
            << std::is_same<decltype(std::declval<Message&>().text()),
                            std::string&>::value
            << ", rvalue yields string = "
            << std::is_same<decltype(std::declval<Message&&>().text()),
                            std::string>::value
            << '\n';
}
