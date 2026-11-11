#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

struct Buffer {
  Buffer() = default;
  Buffer(const Buffer&) = default;
  Buffer(Buffer&&) noexcept = default;
  Buffer& operator=(const Buffer&) = default;
  Buffer& operator=(Buffer&&) noexcept = default;
  int data[4] = {0, 0, 0, 0};
};

// 拷贝与移动都可能抛出，移动构造没有 noexcept。
struct Thrower {
  Thrower() = default;
  Thrower(const Thrower&) {}
  Thrower(Thrower&&) {}
};

int safe_add(int value) noexcept { return value + 1; }
int may_throw(int value) { return value; }
int risky_op();

int relay_safe() noexcept(noexcept(safe_add(1))) { return safe_add(1); }
int relay_risky() noexcept(noexcept(risky_op())) { return 0; }

struct NoThrowMove {
  static int moves;
  static int copies;
  NoThrowMove() = default;
  NoThrowMove(const NoThrowMove&) { ++copies; }
  NoThrowMove(NoThrowMove&&) noexcept { ++moves; }
};
int NoThrowMove::moves = 0;
int NoThrowMove::copies = 0;

struct ThrowingMove {
  static int moves;
  static int copies;
  ThrowingMove() = default;
  ThrowingMove(const ThrowingMove&) { ++copies; }
  ThrowingMove(ThrowingMove&&) { ++moves; }
};
int ThrowingMove::moves = 0;
int ThrowingMove::copies = 0;

int main() {
  // 场景 1：noexcept 进入函数类型，类型特征据此区分两个类。
  std::cout << "[1] nothrow move: Buffer = " << std::boolalpha
            << std::is_nothrow_move_constructible<Buffer>::value
            << ", Thrower = "
            << std::is_nothrow_move_constructible<Thrower>::value << '\n';

  // 场景 2：noexcept(expr) 是编译期运算符，检查函数调用是否会抛。
  std::cout << "[2] noexcept(safe_add) = " << noexcept(safe_add(1))
            << ", noexcept(may_throw) = " << noexcept(may_throw(1)) << '\n';

  // 场景 3：move_if_noexcept 在移动不保证不抛时退回 const 左值引用，
  // 让容器可以安全地选择拷贝。
  bool buffer_rvalue = std::is_rvalue_reference<decltype(std::move_if_noexcept(
      std::declval<Buffer&>()))>::value;
  bool thrower_rvalue = std::is_rvalue_reference<decltype(std::move_if_noexcept(
      std::declval<Thrower&>()))>::value;
  std::cout << "[3] move_if_noexcept rvalue: Buffer = " << buffer_rvalue
            << ", Thrower = " << thrower_rvalue << '\n';

  // 场景 4：异常说明可以条件化，由被调用函数的 noexcept 推导出来。
  std::cout << "[4] relay_safe = " << noexcept(relay_safe())
            << ", relay_risky = " << noexcept(relay_risky()) << '\n';

  // 场景 5：vector 扩容时按元素的 noexcept 移动构造选择移动或拷贝。
  // 用 reserve 强制两次重新分配，计数与实现无关。
  std::vector<NoThrowMove> fast;
  fast.reserve(1);
  fast.emplace_back();
  fast.reserve(3);
  fast.reserve(9);

  std::vector<ThrowingMove> slow;
  slow.reserve(1);
  slow.emplace_back();
  slow.reserve(3);
  slow.reserve(9);

  std::cout << "[5] noexcept move: moves = " << NoThrowMove::moves
            << ", copies = " << NoThrowMove::copies
            << "; throwing move: moves = " << ThrowingMove::moves
            << ", copies = " << ThrowingMove::copies << '\n';
}
