#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

int main() {
  std::mutex mutex;
  std::condition_variable cv;
  std::queue<int> queue;
  bool done = false;

  // 场景 1：生产者在锁内放数据，释放锁后再 notify_one。
  // 消费者用带谓词的 wait，醒来后一次排空队列。
  std::thread producer([&] {
    for (int value = 1; value <= 3; ++value) {
      {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(value);
      }
      cv.notify_one();
    }
    {
      std::lock_guard<std::mutex> lock(mutex);
      done = true;
    }
    cv.notify_one();
  });

  std::thread consumer([&] {
    int count = 0;
    int total = 0;
    while (true) {
      std::unique_lock<std::mutex> lock(mutex);
      cv.wait(lock, [&] { return !queue.empty() || done; });
      while (!queue.empty()) {
        total += queue.front();
        queue.pop();
        ++count;
      }
      if (done) break;
    }
    std::cout << "[1] count = " << count << ", total = " << total << '\n';
  });

  producer.join();
  consumer.join();

  // 场景 2：wait_for 返回状态。没有通知时超时路径稳定可测。
  std::mutex gate_mutex;
  std::condition_variable gate;
  bool opened = false;
  {
    std::unique_lock<std::mutex> lock(gate_mutex);
    auto status = gate.wait_for(lock, std::chrono::milliseconds(10),
                                [&] { return opened; });
    std::cout << "[2] timeout with false predicate = " << std::boolalpha
              << (status == false) << '\n';
  }

  // 场景 3：条件已满足时，wait_for 立即返回 true 而不必等满超时。
  {
    std::lock_guard<std::mutex> lock(gate_mutex);
    opened = true;
  }
  {
    std::unique_lock<std::mutex> lock(gate_mutex);
    auto status = gate.wait_for(lock, std::chrono::milliseconds(10),
                                [&] { return opened; });
    std::cout << "[3] ready predicate = " << status << '\n';
  }
}
