#include <iostream>
#include <string>
#include <utility>

struct Buffer {
  Buffer() = default;
  explicit Buffer(int n) : data(new int[n]), size(n) {
    for (int i = 0; i < n; ++i) data[i] = i + 1;
  }
  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;

  Buffer(Buffer&& other) noexcept
      : data(std::exchange(other.data, nullptr)),
        size(std::exchange(other.size, 0)) {}

  Buffer& operator=(Buffer&& other) noexcept {
    if (this != &other) {
      delete[] data;
      data = std::exchange(other.data, nullptr);
      size = std::exchange(other.size, 0);
    }
    return *this;
  }

  ~Buffer() { delete[] data; }

  bool empty() const { return data == nullptr; }

  int* data = nullptr;
  int size = 0;
};

int main() {
  // exchange 返回旧值，同时把新值写入对象。
  int value = 10;
  const int old = std::exchange(value, 42);
  std::cout << "[1] old = " << old << ", value = " << value << '\n';

  // 用空字符串交换，相当于“取走”原内容。
  std::string text = "hello";
  const std::string taken = std::exchange(text, std::string{});
  std::cout << "[2] taken = " << taken << ", source empty = " << text.empty()
            << '\n';

  // 移动构造把资源交给新对象，并把源对象重置。
  Buffer first(4);
  Buffer second(std::move(first));
  std::cout << "[3] first empty = " << first.empty()
            << ", second size = " << second.size << '\n';

  // 移动赋值同样用 exchange 接管指针，避免重复释放。
  Buffer third;
  third = std::move(second);
  std::cout << "[4] second empty = " << second.empty()
            << ", third sum = " << third.data[0] + third.data[3] << '\n';

  // 消费一次性状态位：读取旧值并在同一次操作里清零。
  bool dirty = true;
  const bool was_dirty = std::exchange(dirty, false);
  std::cout << "[5] was_dirty = " << was_dirty << ", dirty = " << dirty << '\n';
}
