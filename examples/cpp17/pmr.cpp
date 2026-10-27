#include <iostream>
#include <memory_resource>
#include <vector>

int main() {
  std::pmr::monotonic_buffer_resource pool;
  std::pmr::vector<int> values(&pool);

  for (int i = 0; i < 5; ++i) {
    values.push_back(i * i);
  }

  int sum = 0;
  for (int value : values) {
    sum += value;
  }

  std::cout << values.size() << ' ' << sum << '\n';
}
