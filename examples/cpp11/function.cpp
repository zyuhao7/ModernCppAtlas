#include <functional>
#include <iostream>

int add(int a, int b) { return a + b; }

int main() {
  std::function<int(int, int)> op = add;
  int offset = 10;
  op = [offset](int a, int b) { return a + b + offset; };
  std::cout << op(1, 2) << ' ' << (op ? "set" : "empty") << '\n';

  std::function<int(int, int)> empty;
  std::cout << (empty ? "set" : "empty") << '\n';
}
