#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

class Buffer {
 public:
  explicit Buffer(std::vector<int> values) : values_(std::move(values)) {}

  std::size_t size() const { return values_.size(); }

 private:
  std::vector<int> values_;
};

class Token {
 public:
  explicit Token(int value) : value_(value) {}

  Token(const Token&) = delete;
  Token& operator=(const Token&) = delete;

  Token(Token&& other) noexcept : value_(other.value_) { other.value_ = -1; }

  int value() const { return value_; }

 private:
  int value_;
};

struct Counted {
  static int copies;
  static int moves;

  explicit Counted(int value) : value(value) {}

  Counted(const Counted& other) : value(other.value) { ++copies; }

  Counted(Counted&& other) noexcept : value(other.value) {
    other.value = -1;
    ++moves;
  }

  int value;
};

int Counted::copies = 0;
int Counted::moves = 0;

}  // namespace

int main() {
  // 场景 1：sink 参数先按值接收 vector，再用 std::move 把存储交给成员。
  // 调用方传右值时，vector 的缓冲区直接转交，不逐个复制元素。
  std::vector<int> source{1, 2, 3};
  Buffer buffer(std::move(source));
  std::cout << "[1] buffer size = " << buffer.size() << '\n';

  // 场景 2：移动 vector 是常数时间操作，只转交内部缓冲区指针。
  // 无论元素有多少，移动前后 data() 指向同一块内存。
  std::vector<int> large(1000000, 7);
  const int* before = large.data();
  std::vector<int> moved_storage = std::move(large);
  std::cout << "[2] same buffer = " << std::boolalpha
            << (moved_storage.data() == before)
            << ", size = " << moved_storage.size() << '\n';

  // 场景 3：move-only 类型禁用复制，只能通过移动进入容器。
  // 若没有移动构造，push_back(Token(42)) 无法编译。
  std::vector<Token> tokens;
  tokens.push_back(Token(42));
  std::cout << "[3] move-only token value = " << tokens.front().value() << '\n';

  // 场景 4：std::move 只是类型转换。const 对象得到 const T&&，
  // 无法匹配 T&& 移动构造，于是退回拷贝；非 const 对象才会真正移动。
  const Counted fixed{7};
  Counted copied = std::move(fixed);
  Counted movable{8};
  Counted moved = std::move(movable);
  std::cout << "[4] const move copied = " << copied.value
            << ", moved = " << moved.value << ", copies = " << Counted::copies
            << ", moves = " << Counted::moves
            << ", moved-from = " << movable.value << '\n';
}
