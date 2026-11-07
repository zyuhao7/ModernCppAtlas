#include <future>
#include <iostream>
#include <numeric>
#include <vector>

int main() {
  std::future<int> sum = std::async(std::launch::async, [] {
    std::vector<int> values{1, 2, 3, 4};
    return std::accumulate(values.begin(), values.end(), 0);
  });
  std::cout << (sum.valid() ? "valid" : "empty") << ' ' << sum.get() << '\n';

  std::promise<int> promise;
  std::future<int> answer = promise.get_future();
  promise.set_value(42);
  std::cout << answer.get() << '\n';
}
