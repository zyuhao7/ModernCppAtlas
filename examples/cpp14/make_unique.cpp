#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct Session {
  explicit Session(std::string name) : name(std::move(name)) {}
  ~Session() { std::cout << "close " << name << '\n'; }

  std::string name;
};

struct Engine {
  virtual ~Engine() = default;
  virtual const char* kind() const = 0;
};

struct DiskEngine : Engine {
  const char* kind() const override { return "disk"; }
};

std::unique_ptr<Engine> make_engine() { return std::make_unique<DiskEngine>(); }

int main() {
  auto session = std::make_unique<Session>("alpha");
  auto value = std::make_unique<int>(42);
  auto moved = std::move(session);

  std::cout << "[1] moved = " << moved->name << ", value = " << *value
            << ", source empty = " << (session ? 0 : 1) << '\n';

  auto digits = std::make_unique<int[]>(3);
  digits[0] = 1;
  digits[1] = 2;
  digits[2] = 3;
  std::cout << "[2] digits = " << digits[0] << ' ' << digits[1] << ' '
            << digits[2] << ", sum = " << digits[0] + digits[1] + digits[2]
            << '\n';

  std::vector<std::unique_ptr<Session>> sessions;
  sessions.push_back(std::make_unique<Session>("beta"));
  std::cout << "[3] sessions = " << sessions.size()
            << ", front = " << sessions.front()->name << '\n';

  auto engine = make_engine();
  std::cout << "[4] engine = " << engine->kind() << ", derived check = "
            << (dynamic_cast<DiskEngine*>(engine.get()) != nullptr) << '\n';

  std::cout << "[5] scope enter\n";
  {
    auto first = std::make_unique<Session>("one");
    auto second = std::make_unique<Session>("two");
  }
  std::cout << "[5] scope exit\n";
}
