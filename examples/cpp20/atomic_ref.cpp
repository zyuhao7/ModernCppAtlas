#include <atomic>
#include <iostream>

int main() {
  unsigned value = 0;
  std::atomic_ref<unsigned> ref(value);

  ref.fetch_add(2);
  ref.fetch_add(3, std::memory_order_relaxed);
  ref.store(10, std::memory_order_release);

  std::cout << value << ' ' << ref.load(std::memory_order_acquire) << '\n';
}
