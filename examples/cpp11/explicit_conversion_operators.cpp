#include <iostream>

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

int main() {
  Buffer buffer;
  std::cout << (buffer ? "full" : "empty") << '\n';
  buffer.push(7);
  std::cout << (buffer ? "full" : "empty") << '\n';
}
