#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  unsigned value = 0;
  std::atomic_ref<unsigned> ref(value);

  std::vector<std::thread> workers;
  for (int i = 0; i < 4; ++i) {
    workers.emplace_back([&ref] {
      for (int j = 0; j < 1000; ++j) {
        ref.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }
  for (auto& worker : workers) {
    worker.join();
  }

  std::cout << value << ' ' << ref.load(std::memory_order_acquire) << '\n';

  ref.store(10, std::memory_order_release);
  const unsigned previous = ref.exchange(20);
  std::cout << previous << ' ' << value << ' ' << ref.load() << '\n';
}
