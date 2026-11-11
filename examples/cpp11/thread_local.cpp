#include <iostream>
#include <string>
#include <thread>
#include <vector>

thread_local std::string trace;

void note(const std::string& value) {
  trace += value;
  std::cout << "[1] " << trace << '\n';
}

int next_id() {
  static thread_local int id = 0;
  return ++id;
}

struct Session {
  std::string owner;

  ~Session() { std::cout << "[3] session closed for " << owner << '\n'; }
};

void run_session(const std::string& owner) {
  static thread_local Session session;
  session.owner = owner;
}

struct Sample {
  int per_thread;
  int shared;
};

Sample touch() {
  static int shared = 0;
  static thread_local int per_thread = 0;
  ++shared;
  ++per_thread;
  return Sample{per_thread, shared};
}

int main() {
  // 场景 1：命名空间作用域的 thread_local 每个线程一份。
  std::thread worker([] {
    note("w");
    note("x");
  });
  worker.join();
  note("m");

  // 场景 2：块作用域的 static thread_local 在各线程内独立惰性初始化。
  std::thread ids_worker([] {
    std::cout << "[2] worker ids = " << next_id() << ' ' << next_id() << '\n';
  });
  ids_worker.join();
  std::cout << "[2] main ids = " << next_id() << ' ' << next_id() << '\n';

  // 场景 3：非平凡类型的析构函数在线程退出时执行。
  std::thread session_worker([] { run_session("worker"); });
  session_worker.join();

  // 场景 4：普通 static 共享，thread_local 各自独立。
  std::vector<Sample> samples(3);
  for (int i = 0; i < 3; ++i) {
    std::thread runner(
        [&samples, i] { samples[static_cast<std::size_t>(i)] = touch(); });
    runner.join();
  }
  std::cout << "[4] per_thread = " << samples[0].per_thread << ' '
            << samples[1].per_thread << ' ' << samples[2].per_thread
            << ", shared = " << samples[2].shared << '\n';
}
