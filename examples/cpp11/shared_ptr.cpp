#include <iostream>
#include <memory>
#include <utility>

struct Node {
  std::shared_ptr<Node> next;
  std::weak_ptr<Node> parent;
  int value = 0;
  ~Node() { std::cout << "destroy " << value << '\n'; }
};

struct Holder {
  int left;
  int right;
};

int main() {
  auto first = std::make_shared<Node>();
  auto second = std::make_shared<Node>();
  first->value = 1;
  second->value = 2;

  // 场景 1：强引用计数随复制增加。first->next 是 shared_ptr，
  // 因此 second 有两个强持有者：second 自身和 first->next。
  first->next = second;
  second->parent = first;
  std::cout << "[1] first count = " << first.use_count()
            << ", second count = " << second.use_count() << '\n';

  // 场景 2：weak_ptr 不增加计数，用前必须 lock() 拿到临时 shared_ptr。
  if (auto locked = second->parent.lock()) {
    std::cout << "[2] parent value = " << locked->value << '\n';
  }

  // 场景 3：first.reset() 释放 Node 1，它的 next 也随之释放，
  // second 计数退回 1；parent 是 weak_ptr，所以变为 expired。
  first.reset();
  std::cout << "[3] second count = " << second.use_count()
            << ", parent expired = " << std::boolalpha
            << second->parent.expired() << '\n';

  // 场景 4：别名构造共享同一个控制块，却指向成员子对象。
  auto owner = std::make_shared<Holder>(Holder{10, 20});
  std::shared_ptr<int> right(owner, &owner->right);
  std::cout << "[4] alias value = " << *right
            << ", owner count = " << owner.use_count()
            << ", same address = " << (right.get() == &owner->right) << '\n';

  // 场景 5：weak_ptr 在唯一强引用离开作用域后立即失效。
  std::weak_ptr<int> watcher;
  {
    auto temp = std::make_shared<int>(42);
    watcher = temp;
    std::cout << "[5] locked = " << *watcher.lock() << '\n';
  }
  std::cout << "[5] expired after scope = " << watcher.expired() << '\n';
}
