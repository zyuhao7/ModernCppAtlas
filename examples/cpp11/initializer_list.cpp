#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <numeric>
#include <vector>

class Numbers {
 public:
  Numbers(std::initializer_list<int> values) : values_(values) {}

  std::size_t size() const { return values_.size(); }

  int sum() const { return std::accumulate(values_.begin(), values_.end(), 0); }

 private:
  std::vector<int> values_;
};

int total(std::initializer_list<int> values) {
  return std::accumulate(values.begin(), values.end(), 0);
}

// 同时提供普通构造函数与 initializer_list 构造函数，用来观察重载优先级。
struct Widget {
  Widget(int width, int height) : tag("paren"), value(width * height) {}
  Widget(std::initializer_list<int> dims) : tag("brace"), value(0) {
    for (int dim : dims) value += dim;
  }
  const char* tag;
  int value;
};

int main() {
  // 场景 1：构造函数把列表复制进自己的 vector，列表可以随即失效。
  Numbers numbers{1, 2, 3, 4};
  std::cout << "[1] size = " << numbers.size() << ", sum = " << numbers.sum()
            << '\n';

  // 场景 2：函数参数直接接收花括号列表，无需先在调用点建容器。
  std::cout << "[2] total = " << total({10, 20, 30}) << '\n';

  // 场景 3：复制 initializer_list 只复制视图，两个对象指向同一底层数组。
  std::initializer_list<int> values{7, 8, 9};
  auto copy = values;
  std::cout << "[3] copy size = " << copy.size()
            << ", shared storage = " << std::boolalpha
            << (copy.begin() == values.begin()) << '\n';

  // 场景 4：花括号优先选择 initializer_list 构造函数，
  // 想调用两个参数的构造必须改用圆括号。
  Widget paren(3, 4);
  Widget brace{3, 4};
  std::cout << "[4] paren -> " << paren.tag << " value = " << paren.value
            << ", brace -> " << brace.tag << " value = " << brace.value << '\n';
}
