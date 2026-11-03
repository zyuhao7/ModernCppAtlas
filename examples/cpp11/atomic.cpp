#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  std::atomic<int> counter{0};
  std::atomic<bool> ready{false};
  std::vector<std::thread> workers;

  for (int i = 0; i < 4; ++i) {
    workers.emplace_back([&counter, &ready] {
      counter.fetch_add(1, std::memory_order_relaxed);
      ready.store(true, std::memory_order_release);
    });
  }
  for (auto& worker : workers) {
    worker.join();
  }

  std::cout << counter.load() << ' ' << ready.load(std::memory_order_acquire)
            << '\n';

  int expected = counter.load();
  const bool swapped = counter.compare_exchange_strong(expected, 100);
  std::cout << swapped << ' ' << counter.load() << '\n';

  const int previous = counter.exchange(7);
  std::cout << previous << ' ' << counter.load() << '\n';
}
