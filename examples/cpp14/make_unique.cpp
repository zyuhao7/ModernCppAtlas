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

int main() {
  auto session = std::make_unique<Session>("alpha");
  auto value = std::make_unique<int>(42);
  auto moved = std::move(session);

  std::cout << moved->name << ' ' << *value << ' '
            << (session ? "held" : "moved") << '\n';

  auto digits = std::make_unique<int[]>(3);
  digits[0] = 1;
  digits[1] = 2;
  digits[2] = 3;
  std::cout << digits[0] + digits[2] << '\n';

  std::vector<std::unique_ptr<Session>> sessions;
  sessions.push_back(std::make_unique<Session>("beta"));
  std::cout << sessions.front()->name << ' ' << sessions.size() << '\n';
}
