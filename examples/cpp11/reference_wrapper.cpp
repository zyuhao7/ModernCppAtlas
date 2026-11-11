#include <functional>
#include <iostream>
#include <thread>
#include <vector>

namespace {

void bump(int& value) { ++value; }

void bump_by_value(int value) { ++value; }

void add_to(int& total, int value) { total += value; }

struct Accumulator {
  int total = 0;

  void operator()(int value) { total += value; }
};

}  // namespace

int main() {
  // 场景 1：容器不能直接存 int&，但可以存 reference_wrapper<int>。
  // 遍历时通过隐式转换拿到原对象引用，修改会写回 first 与 second。
  int first = 1;
  int second = 2;
  std::vector<std::reference_wrapper<int>> items{first, second};
  for (int& item : items) {
    item *= 10;
  }
  std::cout << "[1] first = " << first << ", second = " << second << '\n';

  // 场景 2：std::thread 先按值保存实参。bump_by_value 只修改线程内的副本；
  // bump(int&) 直接传 value 甚至无法编译，必须用 std::ref 显式保留引用。
  int value = 10;
  std::thread copy_worker(bump_by_value, value);
  copy_worker.join();
  int after_copy = value;
  std::thread ref_worker(bump, std::ref(value));
  ref_worker.join();
  std::cout << "[2] after by-value thread = " << after_copy
            << ", after ref thread = " << value << '\n';

  // 场景 3：std::bind 默认复制实参，std::ref(total) 让绑定对象保存引用，
  // 每次调用都累加到原变量，而不是调用结束后丢弃结果。
  int total = 0;
  auto add = std::bind(add_to, std::ref(total), std::placeholders::_1);
  add(7);
  add(5);
  std::cout << "[3] total after bind = " << total << '\n';

  // 场景 4：包装函数对象时，reference_wrapper 提供 operator()，
  // 调用直接转发给 Accumulator::operator()，状态保存在原对象里。
  Accumulator accumulator;
  std::reference_wrapper<Accumulator> accumulator_ref = std::ref(accumulator);
  accumulator_ref(3);
  std::cout << "[4] accumulator total = " << accumulator.total << '\n';

  // 场景 5：默认的 == 比较被引用对象的值，而不是包装对象或地址；
  // std::cref 返回 reference_wrapper<const int>，只能读不能写。
  int copy = 7;
  int other = 7;
  std::reference_wrapper<int> copy_ref = std::ref(copy);
  std::reference_wrapper<int> other_ref = std::ref(other);
  const int fixed = 42;
  std::reference_wrapper<const int> view = std::cref(fixed);
  std::cout << "[5] refs compare by value = " << std::boolalpha
            << (copy_ref == other_ref) << ", cref value = " << view.get()
            << '\n';

  // 反例（不要运行）：临时对象在语句结束后销毁，包装立即悬空。
  // std::reference_wrapper<const int> bad = std::cref(42);
}
