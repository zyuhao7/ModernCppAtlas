#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Buffer {
 public:
  template <typename T>
  void add(T&& value) {
    store_.push_back(std::forward<T>(value));
  }

  const std::vector<std::string>& items() const { return store_; }

 private:
  std::vector<std::string> store_;
};

int main() {
  Buffer buffer;
  std::string text = "atlas";
  buffer.add(text);
  buffer.add(std::string("moved"));

  for (const auto& item : buffer.items()) {
    std::cout << item << '\n';
  }
  std::cout << text << '\n';
}
