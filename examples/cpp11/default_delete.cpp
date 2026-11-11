#include <cstddef>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

class Point {
 public:
  Point() = default;
  Point(int x, int y) : x_(x), y_(y) {}
  Point(const Point&) = default;
  Point& operator=(const Point&) = default;

  int x() const { return x_; }
  int y() const { return y_; }
  void set_x(int x) { x_ = x; }

 private:
  int x_ = 0;
  int y_ = 0;
};

class NonCopyable {
 public:
  NonCopyable() = default;
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
  virtual ~NonCopyable() = default;
};

class Handle {
 public:
  Handle() = delete;
  explicit Handle(int id) : id_(id) {}

  int id() const { return id_; }

 private:
  int id_;
};

class Buffer {
 public:
  explicit Buffer(std::size_t size) : data_(new int[size]()), size_(size) {}
  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;

  Buffer(Buffer&& other) noexcept : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;
    other.size_ = 0;
  }

  Buffer& operator=(Buffer&& other) noexcept {
    if (this != &other) {
      delete[] data_;
      data_ = other.data_;
      size_ = other.size_;
      other.data_ = nullptr;
      other.size_ = 0;
    }
    return *this;
  }

  ~Buffer() { delete[] data_; }

  std::size_t size() const { return size_; }

 private:
  int* data_ = nullptr;
  std::size_t size_ = 0;
};

int main() {
  // 场景 1：显式默认的拷贝构造保留值语义，副本修改互不影响。
  Point origin(1, 2);
  Point copy = origin;
  copy.set_x(9);
  std::cout << "[1] origin.x = " << origin.x() << ", copy.x = " << copy.x()
            << '\n';

  // 场景 2：默认构造函数仍然执行成员初始化器。
  Point zero;
  std::cout << "[2] default Point = " << zero.x() << ' ' << zero.y() << '\n';

  // 场景 3：删除的拷贝操作直接反映在类型萃取上。
  std::cout << "[3] NonCopyable copyable = "
            << std::is_copy_constructible<NonCopyable>::value
            << ", copy assignable = "
            << std::is_copy_assignable<NonCopyable>::value << '\n';

  // 场景 4：删除的是普通构造函数，不是特殊成员。
  Handle handle(7);
  std::cout << "[4] Handle id = " << handle.id() << ", default constructible = "
            << std::is_default_constructible<Handle>::value << '\n';

  // 场景 5：拷贝删除、移动默认后，移动会把资源交给目标对象。
  Buffer source(3);
  Buffer moved = std::move(source);
  std::cout << "[5] moved.size = " << moved.size()
            << ", source.size = " << source.size()
            << ", copyable = " << std::is_copy_constructible<Buffer>::value
            << ", movable = " << std::is_move_constructible<Buffer>::value
            << '\n';

  // 场景 6：只可移动的类型仍然可以放进 vector。
  std::vector<Buffer> buffers;
  buffers.push_back(Buffer(2));
  buffers.emplace_back(4);
  std::cout << "[6] buffers = " << buffers.size()
            << ", sizes = " << buffers[0].size() << ' ' << buffers[1].size()
            << '\n';
}
