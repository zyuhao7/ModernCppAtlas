#include <iostream>
#include <memory>
#include <string>
#include <utility>

struct Session {
  explicit Session(std::string name) : name(std::move(name)) {}

  std::string name;
};

int main() {
  auto session = std::make_unique<Session>("alpha");
  auto value = std::make_unique<int>(42);
  std::cout << session->name << ' ' << *value << '\n';
}
