#include <atomic>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

std::mutex mutex;

void descend(std::recursive_mutex& mutex, int& depth, int remaining) {
  std::lock_guard<std::recursive_mutex> lock(mutex);
  ++depth;
  if (remaining > 0) {
    descend(mutex, depth, remaining - 1);
  }
}

}  // namespace

int main() {
  // 场景 1：四个线程各自自增一千次。锁把 "读-加-写" 包成临界区，
  // 否则两个线程可能同时读到旧值，最终结果小于 4000。
  int counter = 0;
  auto worker = [&] {
    for (int i = 0; i < 1000; ++i) {
      std::lock_guard<std::mutex> lock(mutex);
      ++counter;
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < 4; ++i) {
    threads.emplace_back(worker);
  }
  for (auto& thread : threads) {
    thread.join();
  }
  std::cout << "[1] counter = " << counter << '\n';

  // 场景 2：两个字段必须一起可见。写线程在同一把锁内更新 left 与 right，
  // 读线程也在锁内复制整个结构体，因此永远看不到更新到一半的快照。
  struct Snapshot {
    int left = 0;
    int right = 0;
  };

  Snapshot shared;
  std::atomic<bool> done{false};
  int mismatches = 0;

  std::thread writer([&] {
    for (int i = 0; i < 10000; ++i) {
      std::lock_guard<std::mutex> lock(mutex);
      ++shared.left;
      ++shared.right;
    }
    done.store(true);
  });

  while (!done.load()) {
    std::lock_guard<std::mutex> lock(mutex);
    Snapshot copy = shared;
    if (copy.left != copy.right) {
      ++mismatches;
    }
  }
  writer.join();
  std::cout << "[2] left = " << shared.left << ", right = " << shared.right
            << ", mismatches = " << mismatches << '\n';

  // 场景 3：lock_guard 在析构时解锁。即使临界区抛出异常，
  // 离开作用域也会释放互斥量，不会把锁永久带走。
  try {
    std::lock_guard<std::mutex> lock(mutex);
    throw std::runtime_error("boom");
  } catch (const std::runtime_error&) {
  }
  // 正是因为异常的时候会正常解锁，所以这里又可以重新获取到锁
  bool released = mutex.try_lock();
  if (released) {
    mutex.unlock();
  }
  std::cout << "[3] lock released after exception = " << std::boolalpha
            << released << '\n';

  // 场景 4：同一线程重复进入同一把锁会死锁；需要递归加锁时
  // 使用 std::recursive_mutex，计数器在每一层都加一。
  std::recursive_mutex recursive;
  int depth = 0;
  descend(recursive, depth, 2);
  std::cout << "[4] recursive depth = " << depth << '\n';
}
