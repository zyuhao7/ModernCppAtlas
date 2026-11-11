#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  // 场景 1：四个线程各做 1000 次 fetch_add，结果确定是 4000。
  // fetch_add 返回旧值，这里只关心最终计数。
  std::atomic<int> counter{0};
  std::vector<std::thread> workers;
  for (int i = 0; i < 4; ++i) {
    workers.emplace_back([&counter] {
      for (int step = 0; step < 1000; ++step) {
        counter.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }
  for (auto& worker : workers) worker.join();
  std::cout << "[1] counter = " << counter.load() << '\n';

  // 场景 2：release 写、acquire 读建立同步，普通变量在标志可见时也可见。
  // 若把两侧都改成 relaxed，编译器与 CPU 都可能让读取看到旧值。
  std::atomic<bool> ready{false};
  int payload = 0;
  std::thread writer([&] {
    payload = 99;
    ready.store(true, std::memory_order_release);
  });
  while (!ready.load(std::memory_order_acquire)) {
  }
  std::cout << "[2] payload after acquire = " << payload << '\n';
  writer.join();

  // 场景 3：compare_exchange 成功时写入新值；失败时把 expected
  // 更新成当前值，因此第二次调用用新的期望值才能成功。
  std::atomic<int> slot{4};
  int expected = 4;
  bool first = slot.compare_exchange_strong(expected, 100);
  int stale = 0;
  bool second = slot.compare_exchange_strong(stale, 7);
  std::cout << "[3] first = " << std::boolalpha << first
            << ", second = " << second << ", stale expected = " << stale
            << ", slot = " << slot.load() << '\n';

  // 场景 4：exchange 原子地读旧值并写新值。
  int previous = counter.exchange(1);
  std::cout << "[4] exchange returned " << previous
            << ", counter now = " << counter.load() << '\n';
}
