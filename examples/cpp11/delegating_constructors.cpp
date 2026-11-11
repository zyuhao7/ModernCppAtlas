#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

int delegated_bodies = 0;

class Logger {
 public:
  Logger() : Logger("info", 0) {}
  explicit Logger(std::string level) : Logger(std::move(level), 0) {}
  Logger(const Logger& other) : Logger(other.level_, other.copies_ + 1) {}

  const std::string& level() const { return level_; }
  int copies() const { return copies_; }

 private:
  Logger(std::string level, int copies)
      : level_(std::move(level)), copies_(copies) {}

  std::string level_;
  int copies_;
};

class Config {
 public:
  Config() : Config(3, "default") {}
  explicit Config(int retries) : Config(retries, "default") {}

  int retries() const { return retries_; }
  const std::string& name() const { return name_; }

 private:
  Config(int retries, std::string name)
      : retries_(retries), name_(std::move(name)) {}

  int retries_;
  std::string name_;
};

class Timeout {
 public:
  Timeout() : Timeout(10) { timeout_ += 5; }
  explicit Timeout(int base) : timeout_(base) {}

  int timeout() const { return timeout_; }

 private:
  int timeout_;
};

class Connection {
 public:
  Connection() : Connection(-1) { ++delegated_bodies; }
  explicit Connection(int port) : port_(port) {
    if (port < 0) throw std::runtime_error("bad port");
  }

  int port() const { return port_; }

 private:
  int port_;
};

int main() {
  // 场景 1：默认构造函数委托到带参数的构造函数。
  Logger fallback;
  std::cout << "[1] level = " << fallback.level()
            << ", copies = " << fallback.copies() << '\n';

  // 场景 2：拷贝构造函数也委托，复制逻辑只有一份。
  Logger copy = fallback;
  std::cout << "[2] copy level = " << copy.level()
            << ", copies = " << copy.copies() << '\n';

  // 场景 3：多个公开构造函数收敛到同一个根构造函数。
  Config config;
  std::cout << "[3] retries = " << config.retries()
            << ", name = " << config.name() << '\n';

  // 场景 4：被委托的构造函数先跑完，委托者的函数体再修改成员。
  Timeout timeout;
  std::cout << "[4] timeout = " << timeout.timeout() << '\n';

  // 场景 5：委托目标抛异常时，委托者的函数体不会执行。
  try {
    Connection connection;
    std::cout << "[5] port = " << connection.port() << '\n';
  } catch (const std::exception& error) {
    std::cout << "[5] caught: " << error.what()
              << ", delegated bodies = " << delegated_bodies << '\n';
  }
}
