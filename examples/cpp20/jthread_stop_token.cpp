#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>

int main() {
  {
    // jthread 把 stop_token 作为首参传给线程函数。
    std::atomic<bool> got_token{false};
    std::jthread worker(
        [&](std::stop_token token) { got_token = token.stop_possible(); });
    worker.join();
    std::cout << "[1] token delivered = " << got_token
              << ", stop possible = " << worker.get_stop_token().stop_possible()
              << '\n';
  }

  {
    // request_stop 第一次返回 true，重复请求返回 false。
    std::jthread worker([](std::stop_token token) {
      while (!token.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });
    const bool first = worker.request_stop();
    const bool second = worker.request_stop();
    std::cout << "[2] first request = " << first
              << ", second request = " << second
              << ", requested = " << worker.get_stop_token().stop_requested()
              << '\n';
  }

  {
    // condition_variable_any 的 wait 重载在停止请求时返回，无需额外通知。
    std::mutex mutex;
    std::condition_variable_any ready;
    std::jthread worker([&](std::stop_token token) {
      std::unique_lock lock(mutex);
      ready.wait(lock, token, [] { return false; });
    });
    worker.request_stop();
    std::cout << "[3] joinable after request = " << worker.joinable() << '\n';
  }

  {
    // stop_callback 在 request_stop 的线程上同步执行。
    std::stop_source source;
    std::string where;
    const std::thread::id caller = std::this_thread::get_id();
    std::stop_callback callback(source.get_token(), [&] {
      where = std::this_thread::get_id() == caller ? "caller" : "other";
    });
    const bool stopped = source.request_stop();
    std::cout << "[4] callback ran on = " << where
              << ", request_stop = " << stopped << '\n';
  }

  {
    // 析构自动请求停止并 join；观察到线程确实跑过再退出作用域。
    std::atomic<int> ticks{0};
    {
      std::jthread worker([&](std::stop_token token) {
        while (!token.stop_requested()) {
          ++ticks;
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      });
      while (ticks.load() == 0) {
        std::this_thread::yield();
      }
    }
    std::cout << "[5] joined at scope exit = "
              << (ticks.load() > 0 ? "yes" : "no") << '\n';
  }
}
