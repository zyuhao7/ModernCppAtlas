#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

int main() {
  std::once_flag flag;
  int initialized = 0;
  auto init = [&initialized] { ++initialized; };

  std::vector<std::thread> workers;
  for (int i = 0; i < 8; ++i) {
    workers.emplace_back([&] { std::call_once(flag, init); });
  }
  for (auto& worker : workers) {
    worker.join();
  }

  std::cout << initialized << '\n';
}
