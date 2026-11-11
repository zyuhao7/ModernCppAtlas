#include <iostream>
#include <string>
#include <vector>

// 聚合体：成员按声明顺序绑定，空花括号做值初始化。
struct Point {
  int x;
  int y;
};

// 同时提供 initializer_list 构造与普通构造，用来观察重载优先级。
struct Range {
  explicit Range(std::initializer_list<int> values)
      : tag("brace"), first(0), last(0) {
    if (values.size() == 2) {
      auto it = values.begin();
      first = *it;
      last = *(it + 1);
    }
  }
  Range(int from, int to) : tag("paren"), first(from), last(to) {}

  const char* tag;
  int first;
  int last;
};

Point make_point(int x, int y) { return {x, y}; }

int main() {
  // 场景 1：空花括号把聚合体成员零初始化，避免读到未定义值。
  Point origin{};
  std::cout << "[1] origin = " << origin.x << ' ' << origin.y << '\n';

  // 场景 2：非空花括号按声明顺序绑定成员，顺序即接口。
  Point corner{3, 4};
  Point returned = make_point(5, 6);
  std::cout << "[2] corner = " << corner.x << ' ' << corner.y
            << ", returned = " << returned.x << ' ' << returned.y << '\n';

  // 场景 3：花括号与圆括号在容器上语义完全不同。
  // vector{10, 20} 是两个元素，vector(10, 20) 是十个 20。
  std::vector<int> two_elements{10, 20};
  std::vector<int> ten_twenties(10, 20);
  std::cout << "[3] brace size = " << two_elements.size()
            << ", paren size = " << ten_twenties.size()
            << ", paren value = " << ten_twenties.front() << '\n';

  // 场景 4：initializer_list 构造函数优先于普通构造函数。
  Range braced{1, 9};
  Range parenthesized(1, 9);
  std::cout << "[4] braced -> " << braced.tag << " [" << braced.first << ", "
            << braced.last << "], paren -> " << parenthesized.tag << " ["
            << parenthesized.first << ", " << parenthesized.last << "]\n";

  // 场景 5：花括号构造标准库类型，避免圆括号被误读成其它重载。
  std::string label{"atlas"};
  std::vector<std::string> words{"a", "b", "c"};
  std::cout << "[5] label = " << label << ", words = " << words.size() << '\n';

  // 窄化检查只在花括号下生效，且是编译期错误，因此此处不写可执行代码。
  // int narrowed{3.9};   // 编译失败
  // char too_wide{1000}; // 编译失败
  // int accepted(3.9);   // 合法，静默截断为 3
}
