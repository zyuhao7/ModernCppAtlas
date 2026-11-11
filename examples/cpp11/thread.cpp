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

void bump_by_value(int value) { value += 1; }
void bump_by_reference(int& value) { value += 1; }

int main() {
  // 场景 1：四个线程各算一段区间，std::ref 把结果写回 partial[i]。
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

  std::cout << "[1] workers = " << threads.size()
            << ", joinable = " << std::boolalpha << threads.front().joinable()
            << '\n';

  for (auto& worker : threads) {
    worker.join();
  }

  long long total = 0;
  for (long long value : partial) {
    total += value;
  }
  std::cout << "[1] first = " << partial.front()
            << ", last = " << partial.back() << ", total = " << total << '\n';

  // 场景 2：线程参数默认按值存进线程内部，改写影响的是副本。
  int value = 1;
  std::thread by_value(bump_by_value, value);
  by_value.join();
  std::cout << "[2] after by-value = " << value << '\n';

  // 场景 3：想共享同一对象必须显式 std::ref；直接传 int& 无法通过编译。
  std::thread by_reference(bump_by_reference, std::ref(value));
  by_reference.join();
  std::cout << "[3] after std::ref = " << value << '\n';

  // 场景 4：lambda 按引用捕获局部变量，join 后结果可见。
  long long doubled = 0;
  std::thread lambda_worker([&doubled] { doubled = 21 * 2; });
  lambda_worker.join();
  std::cout << "[4] lambda result = " << doubled << '\n';

  // 场景 5：hardware_concurrency 只是提示，标准允许返回 0，
  // 因此输出分类而不是具体数值，避免把机器差异写进快照。
  unsigned hint = std::thread::hardware_concurrency();
  std::cout << "[5] hardware_concurrency "
            << (hint == 0 ? "unavailable" : "available") << '\n';
}
