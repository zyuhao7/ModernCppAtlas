#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

int main() {
  // 场景 1：async 启动后台求和，get() 等待并取回结果；
  // get() 之后 future 变为 invalid。
  std::future<int> sum = std::async(std::launch::async, [] {
    std::vector<int> values{1, 2, 3, 4};
    return std::accumulate(values.begin(), values.end(), 0);
  });
  std::cout << "[1] valid before get = " << std::boolalpha << sum.valid()
            << ", result = " << sum.get()
            << ", valid after get = " << sum.valid() << '\n';

  // 场景 2：promise 写入后，等待方的 future 才就绪。
  std::promise<int> promise;
  std::future<int> answer = promise.get_future();
  std::thread producer([&promise] { promise.set_value(42); });
  std::cout << "[2] answer = " << answer.get() << '\n';
  producer.join();

  // 场景 3：packaged_task 把可调用对象与共享状态绑定，
  // 可以先构造，稍后再交给线程执行。
  std::packaged_task<int(int, int)> task([](int a, int b) { return a * b; });
  std::future<int> product = task.get_future();
  std::thread runner(std::move(task), 6, 7);
  std::cout << "[3] product = " << product.get() << '\n';
  runner.join();

  // 场景 4：任务中的异常存进共享状态，get() 时重新抛出。
  std::future<int> broken = std::async(std::launch::async, []() -> int {
    throw std::runtime_error("task failed");
  });
  try {
    broken.get();
  } catch (const std::exception& error) {
    std::cout << "[4] caught = " << error.what() << '\n';
  }

  // 场景 5：shared_future 可以复制给多个消费者，get() 可重复调用。
  std::promise<std::string> shared_promise;
  std::shared_future<std::string> shared = shared_promise.get_future().share();
  shared_promise.set_value("atlas");
  std::cout << "[5] first = " << shared.get() << ", second = " << shared.get()
            << '\n';

  // 场景 6：wait_for 在结果就绪前返回 timeout 或 deferred 状态。
  std::promise<int> slow;
  std::future<int> pending = slow.get_future();
  auto status = pending.wait_for(std::chrono::milliseconds(1));
  std::cout << "[6] timeout = " << std::boolalpha
            << (status == std::future_status::timeout) << '\n';
  slow.set_value(1);
  (void)pending.get();
}
