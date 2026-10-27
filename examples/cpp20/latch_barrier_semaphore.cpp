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

  std::barrier rendezvous(2);
  int phase = 0;
  std::thread partner([&] {
    ++phase;
    rendezvous.arrive_and_wait();
  });
  ++phase;
  rendezvous.arrive_and_wait();
  partner.join();

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
  for (auto &thread : workers) {
    thread.join();
  }

  std::cout << latch_value << ' ' << phase << ' ' << finished << '\n';
}
