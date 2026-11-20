#include <cstddef>
#include <iostream>
#include <memory>
#include <new>

namespace {

struct WithConstMember {
  const int id;
  int payload;
};

}  // namespace

int main() {
  // [1] placement-new into aligned storage, then launder before use
  alignas(WithConstMember) std::byte storage[sizeof(WithConstMember)];
  new (storage) WithConstMember{7, 100};
  WithConstMember* obj =
      std::launder(reinterpret_cast<WithConstMember*>(storage));
  std::cout << "[1] id = " << obj->id << ", payload = " << obj->payload << '\n';

  // [2] launder points at the same address as the raw storage
  std::cout << "[2] same address = "
            << (static_cast<void*>(obj) == static_cast<void*>(storage)) << '\n';

  // [3] mutate through the laundered pointer
  obj->payload = 200;
  WithConstMember* again =
      std::launder(reinterpret_cast<WithConstMember*>(storage));
  std::cout << "[3] payload = " << again->payload << '\n';

  // [4] reuse the storage: destroy and construct a new object, launder again
  std::destroy_at(obj);
  new (storage) WithConstMember{9, 300};
  WithConstMember* fresh =
      std::launder(reinterpret_cast<WithConstMember*>(storage));
  std::cout << "[4] id = " << fresh->id << ", payload = " << fresh->payload
            << '\n';

  // [5] construct and find each independent object in the buffer
  alignas(int) std::byte cells[sizeof(int) * 3];
  int* squares[3];
  for (int i = 0; i < 3; ++i) {
    auto* slot = cells + i * sizeof(int);
    new (slot) int(i * i);
    squares[i] = std::launder(reinterpret_cast<int*>(slot));
  }
  std::cout << "[5] squares = " << *squares[0] << ' ' << *squares[1] << ' '
            << *squares[2] << '\n';

  // [6] destroy each object through its own pointer
  for (int* square : squares) std::destroy_at(square);
  std::destroy_at(fresh);
  std::cout << "[6] destroyed all objects\n";
}
