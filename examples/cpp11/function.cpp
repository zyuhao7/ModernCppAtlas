#include <functional>
#include <iostream>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

struct Accumulator {
  int total = 0;
  void add(int value) { total += value; }
  int read() const { return total; }
};

int main() {
  // 场景 1：同一签名装下函数指针与带捕获的 lambda。
  std::function<int(int, int)> op = add;
  std::cout << "[1] function pointer = " << op(2, 3) << ", target is raw = "
            << (op.target_type() == typeid(int (*)(int, int)) ? "true"
                                                              : "false")
            << '\n';
  int offset = 10;
  op = [offset](int a, int b) { return a + b + offset; };
  std::cout << "[1] lambda = " << op(2, 3) << ", target is raw = "
            << (op.target_type() == typeid(int (*)(int, int)) ? "true"
                                                              : "false")
            << '\n';

  // 场景 2：按值复制，两份各自持有同一个目标。
  std::function<int(int, int)> copy = op;
  std::cout << "[2] original = " << op(1, 2) << ", copy = " << copy(1, 2)
            << '\n';

  // 场景 3：空 function 可判空，直接调用抛 std::bad_function_call。
  std::function<int(int, int)> empty;
  std::cout << "[3] empty is set = " << (empty ? "true" : "false");
  try {
    empty(1, 2);
  } catch (const std::bad_function_call&) {
    std::cout << ", call throws bad_function_call";
  }
  std::cout << '\n';

  // 场景 4：std::bind 固定部分参数，占位符接收剩余实参。
  auto plus_five = std::bind(add, 5, std::placeholders::_1);
  std::cout << "[4] bind(add, 5, _1)(7) = " << plus_five(7) << '\n';

  // 场景 5：std::mem_fn 把成员函数包装成可调用对象。
  Accumulator acc;
  auto add_to_acc = std::mem_fn(&Accumulator::add);
  add_to_acc(acc, 4);
  add_to_acc(acc, 6);
  std::cout << "[5] mem_fn total = " << acc.read() << '\n';

  // 场景 6：同签名的不同可调用对象放进容器，运行期按名字分派。
  std::vector<std::pair<std::string, std::function<int(int, int)>>> table;
  table.push_back(
      std::make_pair(std::string("add"), std::function<int(int, int)>(add)));
  table.push_back(std::make_pair(std::string("mul"),
                                 std::function<int(int, int)>(multiply)));
  for (std::size_t i = 0; i < table.size(); ++i) {
    std::cout << "[6] " << table[i].first
              << "(2, 3) = " << table[i].second(2, 3) << '\n';
  }
}
