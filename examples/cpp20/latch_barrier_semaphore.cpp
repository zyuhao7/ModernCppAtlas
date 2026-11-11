#include <atomic>
#include <barrier>
#include <iostream>
#include <latch>
#include <semaphore>
#include <thread>
#include <vector>

int main() {
  std::latch start(1);
  int latch_value = 0;
  std::thread worker([&] {
    start.wait();
    latch_value = 42;
  });
  start.count_down();
  worker.join();
  std::cout << "[1] latch value = " << latch_value << '\n';

  std::latch done(3);
  std::atomic<int> total{0};
  std::vector<std::thread> waiters;
  for (int i = 0; i < 3; ++i) {
    waiters.emplace_back([&] {
      total.fetch_add(1);
      done.count_down();
    });
  }
  done.wait();
  for (auto& thread : waiters) {
    thread.join();
  }
  std::cout << "[2] latch waiters = 3, total = " << total.load() << '\n';

  std::barrier rendezvous(2);
  std::atomic<int> arrivals{0};
  std::thread partner([&] {
    arrivals.fetch_add(1);
    rendezvous.arrive_and_wait();
    arrivals.fetch_add(1);
    rendezvous.arrive_and_wait();
  });
  arrivals.fetch_add(1);
  rendezvous.arrive_and_wait();
  arrivals.fetch_add(1);
  rendezvous.arrive_and_wait();
  partner.join();
  std::cout << "[3] barrier arrivals = " << arrivals.load() << '\n';

  std::counting_semaphore<2> permits(2);
  std::atomic<int> finished{0};
  std::vector<std::thread> workers;
  for (int i = 0; i < 4; ++i) {
    workers.emplace_back([&] {
      permits.acquire();
      finished.fetch_add(1);
      permits.release();
    });
  }
  for (auto& thread : workers) {
    thread.join();
  }
  std::cout << "[4] semaphore permits = 2, finished = " << finished.load()
            << '\n';

  std::binary_semaphore signal(0);
  int signaled = 0;
  std::thread producer([&] {
    signaled = 5;
    signal.release();
  });
  signal.acquire();
  producer.join();
  std::cout << "[5] signaled value = " << signaled << '\n';
}
