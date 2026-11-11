#include <cstddef>
#include <iostream>
#include <memory_resource>
#include <new>
#include <string>
#include <vector>

int main() {
  std::byte buffer[1024];
  std::pmr::monotonic_buffer_resource pool(buffer, sizeof(buffer));

  std::pmr::vector<int> values(&pool);
  for (int i = 0; i < 5; ++i) {
    values.push_back(i * i);
  }

  std::cout << "[1] values =";
  for (int value : values) {
    std::cout << ' ' << value;
  }
  std::cout << ", size = " << values.size() << '\n';

  int sum = 0;
  for (int value : values) {
    sum += value;
  }
  std::cout << "[2] sum = " << sum << '\n';

  std::pmr::string text("atlas", &pool);
  std::cout << "[3] text = " << text << ", size = " << text.size() << '\n';
  std::cout << "[4] same resource = "
            << (values.get_allocator().resource() == &pool) << '\n';

  std::pmr::polymorphic_allocator<int> alloc(&pool);
  int* raw = alloc.allocate(1);
  new (raw) int(7);
  std::cout << "[5] raw = " << *raw << '\n';
}
