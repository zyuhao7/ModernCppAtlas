#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

int classify(std::string&) { return 1; }
int classify(const std::string&) { return 2; }

struct Widget {
  int size() { return 10; }
  int size() const { return 20; }
};

}  // namespace

int main() {
  int value = 7;

  // [1] the returned type is a const lvalue reference
  static_assert(std::is_same_v<decltype(std::as_const(value)), const int&>);
  std::cout
      << "[1] as_const is const int& = " << std::boolalpha
      << std::is_same_v<decltype(std::as_const(value)), const int&> << '\n';

  // [2] overload resolution picks the const overload
  std::string text = "hello";
  std::cout << "[2] overload: non-const = " << classify(text)
            << ", const = " << classify(std::as_const(text)) << '\n';

  // [3] no copy: the address is the same object
  std::cout << "[3] same address = " << (&std::as_const(value) == &value)
            << '\n';

  // [4] member overload selection follows the added const
  Widget widget;
  std::cout << "[4] size non-const = " << widget.size()
            << ", const = " << std::as_const(widget).size() << '\n';

  // [5] range-for over a read-only view without copying the container
  std::vector<int> values{1, 2, 3};
  int sum = 0;
  for (const int& item : std::as_const(values)) sum += item;
  std::cout << "[5] sum = " << sum << ", size = " << values.size() << '\n';

  // [6] type deduction keeps the const qualification
  auto&& ref = std::as_const(text);
  std::cout << "[6] string is const = "
            << std::is_const_v<std::remove_reference_t<decltype(ref)>> << '\n';
}
