#include <iostream>
#include <type_traits>
#include <utility>

int counter = 0;

int& slot() { return counter; }

// auto 推导按值返回，引用被丢弃。
auto copy_of_slot() { return slot(); }

// decltype(auto) 按 decltype 规则推导，引用被保留。
decltype(auto) ref_to_slot() { return slot(); }

// 返回表达式带括号时，decltype 得到引用类型。
decltype(auto) parenthesized() { return (counter); }

// 返回未加括号的 id-expression 时，decltype 得到变量声明类型。
decltype(auto) plain() { return counter; }

template <class F, class... Args>
decltype(auto) forward_call(F&& f, Args&&... args) {
  return std::forward<F>(f)(std::forward<Args>(args)...);
}

int main() {
  std::cout << "[1] auto return is int = "
            << std::is_same<decltype(copy_of_slot()), int>::value << '\n';
  std::cout << "[2] decltype(auto) return is int& = "
            << std::is_same<decltype(ref_to_slot()), int&>::value << '\n';

  decltype(auto) alias = ref_to_slot();
  alias = 7;
  std::cout << "[3] counter after writing through alias = " << counter << '\n';

  std::cout << "[4] return (counter) is int& = "
            << std::is_same<decltype(parenthesized()), int&>::value << '\n';
  std::cout << "[5] return counter is int = "
            << std::is_same<decltype(plain()), int>::value << '\n';
  std::cout << "[6] forwarding wrapper keeps int& = "
            << std::is_same<decltype(forward_call(slot)), int&>::value << '\n';
}
