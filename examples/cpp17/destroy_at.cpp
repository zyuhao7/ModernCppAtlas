#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <new>

struct Item {
  explicit Item(int& counter) noexcept : destroyed(counter) {}
  int& destroyed;
  ~Item() { ++destroyed; }
};

int main() {
  int destroyed = 0;
  alignas(Item) std::byte storage[sizeof(Item)];
  auto* item = new (storage) Item{destroyed};
  std::destroy_at(item);
  std::cout << "[1] first destruction = " << destroyed << '\n';

  // 在同一块存储上开始新的对象生命周期，使用构造返回的指针。
  auto* next = new (storage) Item{destroyed};
  std::destroy_at(next);
  assert(destroyed == 2);
  std::cout << "[2] after slot reuse = " << destroyed << '\n';

  // 容量是 3，但只有前两个槽位构造了对象。
  std::allocator<Item> allocator;
  auto* slots = allocator.allocate(3);
  new (slots) Item{destroyed};
  new (slots + 1) Item{destroyed};
  std::destroy_n(slots, 2);
  allocator.deallocate(slots, 3);
  assert(destroyed == 4);
  std::cout << "[3] after destroying two live slots = " << destroyed << '\n';

  {
    auto destroy_only = [](Item* p) { std::destroy_at(p); };
    std::unique_ptr<Item, decltype(destroy_only)> guard(
        new (storage) Item{destroyed}, destroy_only);
  }
  assert(destroyed == 5);
  std::cout << "[4] after scope exit = " << destroyed << '\n';
}
