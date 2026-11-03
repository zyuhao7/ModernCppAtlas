#include <functional>
#include <iostream>
#include <typeinfo>

int add(int a, int b) { return a + b; }

int main() {
  std::function<int(int, int)> op = add;
  int offset = 10;
  op = [offset](int a, int b) { return a + b + offset; };

  auto copy = op;
  std::cout << op(1, 2) << ' ' << copy(1, 2) << ' '
            << (op.target_type() == typeid(int (*)(int, int)) ? "raw"
                                                              : "lambda")
            << '\n';

  std::cout << (op ? "set" : "empty") << ' ';
  op = nullptr;
  std::cout << (op ? "set" : "empty") << '\n';

  auto bind_add = std::bind(add, 5, std::placeholders::_1);
  std::cout << bind_add(7) << '\n';
}
