#include <iostream>
#include <memory>
#include <utility>

struct Connection {
  explicit Connection(int id) : id(id) {}
  ~Connection() { std::cout << "close " << id << '\n'; }
  int id;
};

std::unique_ptr<Connection> connect(int id) {
  return std::unique_ptr<Connection>(new Connection(id));
}

int main() {
  std::unique_ptr<Connection> primary = connect(1);
  std::unique_ptr<Connection> backup = connect(2);
  primary = std::move(backup);
  std::cout << (backup ? "held" : "moved") << '\n';
}
