#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

struct Counter {
  int value = 0;

  void increment() { ++value; }
};

int main() {
  // 场景 1：八个线程并发调用，初始化函数只执行一次。
  std::once_flag once_concurrent;
  int initialized = 0;
  std::vector<std::thread> workers;
  for (int i = 0; i < 8; ++i) {
    workers.emplace_back(
        [&] { std::call_once(once_concurrent, [&] { ++initialized; }); });
  }
  for (auto& worker : workers) {
    worker.join();
  }
  std::cout << "[1] threads = 8, init count = " << initialized << '\n';

  // 场景 2：初始化抛异常时标志不会标记完成，下一次调用会重试。
  std::once_flag once_flaky;
  int attempts = 0;
  auto flaky = [&] {
    ++attempts;
    if (attempts == 1) throw std::runtime_error("first try fails");
  };
  try {
    std::call_once(once_flaky, flaky);
  } catch (const std::exception& error) {
    std::cout << "[2] attempt 1 threw: " << error.what() << '\n';
  }
  std::call_once(once_flaky, flaky);
  std::cout << "[2] attempts = " << attempts
            << ", finished = " << (attempts == 2 ? "yes" : "no") << '\n';

  // 场景 3：成员函数和对象指针一起转发给 call_once。
  std::once_flag once_member;
  Counter counter;
  std::vector<std::thread> callers;
  for (int i = 0; i < 4; ++i) {
    callers.emplace_back(
        [&] { std::call_once(once_member, &Counter::increment, &counter); });
  }
  for (auto& caller : callers) {
    caller.join();
  }
  std::cout << "[3] member init count = " << counter.value << '\n';

  // 场景 4：同一个标志上的后续可调用对象不会再执行。
  std::once_flag once_winner;
  std::string winner;
  std::call_once(once_winner, [&] { winner = "first"; });
  std::call_once(once_winner, [&] { winner = "second"; });
  std::cout << "[4] winner = " << winner << '\n';

  // 场景 5：初始化完成后，写入的数据对等待的线程可见。
  std::once_flag once_visible;
  int shared = 0;
  std::thread writer(
      [&] { std::call_once(once_visible, [&] { shared = 42; }); });
  writer.join();
  std::cout << "[5] shared after join = " << shared << '\n';

  // 场景 6：每个实例持有自己的 once_flag，各自只初始化一次。
  struct Lazy {
    std::once_flag flag;
    int loads = 0;
    int value = 0;

    int load() {
      std::call_once(flag, [this] {
        ++loads;
        value = 7;
      });
      return value;
    }
  };
  Lazy first;
  Lazy second;
  std::cout << "[6] first = " << first.load() << ", second = " << second.load()
            << ", loads = " << first.loads << ' ' << second.loads << '\n';
}
