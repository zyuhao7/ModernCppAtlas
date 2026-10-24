#include <coroutine>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

template <typename T>
class Generator {
 public:
  struct promise_type {
    T current{};

    Generator get_return_object() {
      return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    std::suspend_always yield_value(T value) {
      current = std::move(value);
      return {};
    }
    void return_void() noexcept {}
    void unhandled_exception() { std::terminate(); }
  };

  explicit Generator(std::coroutine_handle<promise_type> handle)
      : handle_(handle) {}
  Generator(Generator&& other) noexcept
      : handle_(std::exchange(other.handle_, {})) {}
  Generator(const Generator&) = delete;
  Generator& operator=(const Generator&) = delete;
  ~Generator() {
    if (handle_) handle_.destroy();
  }

  bool next() {
    handle_.resume();
    return !handle_.done();
  }
  const T& value() const { return handle_.promise().current; }

 private:
  std::coroutine_handle<promise_type> handle_;
};

template <typename T>
class Task {
 public:
  struct promise_type {
    T value{};
    std::exception_ptr error;
    std::coroutine_handle<> continuation{};

    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept { return {}; }

    struct FinalAwaiter {
      bool await_ready() noexcept { return false; }
      std::coroutine_handle<> await_suspend(
          std::coroutine_handle<promise_type> handle) noexcept {
        auto continuation = handle.promise().continuation;
        return continuation ? continuation : std::noop_coroutine();
      }
      void await_resume() noexcept {}
    };

    FinalAwaiter final_suspend() noexcept { return {}; }
    void return_value(T result) { value = std::move(result); }
    void unhandled_exception() { error = std::current_exception(); }
  };

  explicit Task(std::coroutine_handle<promise_type> handle)
      : handle_(handle) {}
  Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;
  ~Task() {
    if (handle_) handle_.destroy();
  }

  struct Awaiter {
    std::coroutine_handle<promise_type> handle;

    bool await_ready() const noexcept { return !handle || handle.done(); }
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<> continuation) noexcept {
      handle.promise().continuation = continuation;
      return handle;
    }
    T await_resume() {
      if (handle.promise().error) {
        std::rethrow_exception(handle.promise().error);
      }
      return std::move(handle.promise().value);
    }
  };

  Awaiter operator co_await() && { return Awaiter{handle_}; }

  T run() {
    handle_.resume();
    if (handle_.promise().error) {
      std::rethrow_exception(handle_.promise().error);
    }
    return std::move(handle_.promise().value);
  }

 private:
  std::coroutine_handle<promise_type> handle_;
};

Generator<int> range(int first, int last) {
  for (int value = first; value < last; ++value) {
    co_yield value;
  }
}

Task<int> fetch_value(int seed) { co_return seed * 3; }

Task<int> compute(int seed) {
  const int first = co_await fetch_value(seed);
  const int second = co_await fetch_value(first);
  co_return first + second;
}

Task<int> fail() {
  throw std::runtime_error("boom");
  co_return 0;
}

int main() {
  std::cout << "range:";
  auto numbers = range(1, 5);
  while (numbers.next()) {
    std::cout << ' ' << numbers.value();
  }
  std::cout << '\n';

  std::cout << "task: " << compute(2).run() << '\n';

  try {
    (void)fail().run();
  } catch (const std::exception& error) {
    std::cout << "caught: " << error.what() << '\n';
  }
}
