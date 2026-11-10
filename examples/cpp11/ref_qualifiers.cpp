#include <iostream>
#include <string>
#include <utility>

class Message {
 public:
  std::string& text() & { return text_; }
  std::string text() && { return std::move(text_); }

 private:
  std::string text_ = "atlas";
};

int main() {
  Message message;
  std::cout << message.text() << '\n';
  std::string moved = Message{}.text();
  std::cout << moved << '\n';
}
