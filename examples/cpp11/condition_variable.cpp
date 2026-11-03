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
      if (done) {
        break;
      }
    }
    std::cout << count << ' ' << total << '\n';
  });

  producer.join();
  consumer.join();
}
