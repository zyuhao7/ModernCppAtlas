#include <iostream>
#include <memory>

struct Node {
  std::shared_ptr<Node> next;
  std::weak_ptr<Node> parent;
  int value = 0;
};

int main() {
  auto first = std::make_shared<Node>();
  auto second = std::make_shared<Node>();
  first->value = 1;
  second->value = 2;
  first->next = second;
  second->parent = first;

  std::cout << first.use_count() << ' ' << second.use_count() << '\n';
  if (auto locked = second->parent.lock()) {
    std::cout << locked->value << '\n';
  }
  first.reset();
  std::cout << (second->parent.expired() ? "expired" : "alive") << '\n';
}
