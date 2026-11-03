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

  int sum = 0;
  for (int value : values) {
    sum += value;
  }

  std::pmr::string text("atlas", &pool);

  std::pmr::polymorphic_allocator<int> alloc(&pool);
  int* raw = alloc.allocate(1);
  new (raw) int(7);

  std::cout << values.size() << ' ' << sum << ' ' << *raw << '\n';
  std::cout << text << ' ' << text.size() << ' '
            << (values.get_allocator().resource() == &pool) << '\n';
}
