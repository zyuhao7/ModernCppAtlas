#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

class Buffer {
 public:
  Buffer() = default;
  Buffer(const Buffer&) = default;
  Buffer& operator=(const Buffer&) = default;
  Buffer(Buffer&&) noexcept = default;
  Buffer& operator=(Buffer&&) noexcept = default;
};

int may_throw(int value) { return value; }

int main() {
  std::cout << std::is_nothrow_move_constructible<Buffer>::value << ' '
            << noexcept(std::declval<Buffer>()) << '\n';
  std::cout << noexcept(may_throw(1)) << '\n';

  std::vector<Buffer> buffers;
  buffers.reserve(2);
  buffers.emplace_back();
  buffers.emplace_back();
  std::cout << buffers.size() << ' ' << buffers.capacity() << '\n';
}
