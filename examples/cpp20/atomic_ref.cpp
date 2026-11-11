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

  std::cout << "[1] value = " << value
            << ", loaded = " << ref.load(std::memory_order_acquire) << '\n';

  ref.store(10, std::memory_order_release);
  const unsigned previous = ref.exchange(20);
  std::cout << "[2] previous = " << previous << ", after exchange = " << value
            << '\n';

  unsigned expected = 20;
  const bool success = ref.compare_exchange_strong(expected, 30);
  expected = 99;
  const bool failed = ref.compare_exchange_strong(expected, 40);
  std::cout << "[3] success = " << success << ", value = " << value
            << ", failed = " << failed << ", expected = " << expected << '\n';

  std::atomic_ref<unsigned> copied = ref;
  copied.fetch_sub(5);
  std::cout << "[4] copied sees = " << ref.load() << ", original = " << value
            << '\n';

  unsigned flags = 4;
  std::atomic_ref<unsigned> view(flags);
  view.fetch_or(1);
  view.fetch_and(~4u);
  std::cout << "[5] flags = " << flags << '\n';
}
