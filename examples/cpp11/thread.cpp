#include <functional>
#include <iostream>
#include <thread>
#include <vector>

void accumulate(int from, int to, long long& out) {
  long long sum = 0;
  for (int value = from; value <= to; ++value) {
    sum += value;
  }
  out = sum;
}

int main() {
  const int workers = 4;
  const int per_worker = 25;
  std::vector<long long> partial(workers, 0);
  std::vector<std::thread> threads;
  threads.reserve(workers);

  for (int i = 0; i < workers; ++i) {
    const int from = i * per_worker + 1;
    const int to = from + per_worker - 1;
    threads.emplace_back(accumulate, from, to, std::ref(partial[i]));
  }

  std::cout << threads.size() << ' ' << (threads.front().joinable() ? 1 : 0)
            << '\n';

  for (auto& worker : threads) {
    worker.join();
  }

  long long total = 0;
  for (long long value : partial) {
    total += value;
  }

  std::cout << partial.front() << ' ' << partial.back() << ' ' << total << '\n';

  long long doubled = 0;
  std::thread lambda_worker([&doubled] { doubled = 21 * 2; });
  lambda_worker.join();
  std::cout << doubled << '\n';
}
