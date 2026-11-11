#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <tuple>

int sum(const std::array<int, 5>& values) {
  int total = 0;
  for (int value : values) total += value;
  return total;
}

int main() {
  std::array<int, 5> values{{3, 1, 4, 1, 5}};

  // 场景 1：长度与首尾元素；按引用传给函数不会退化成指针。
  std::cout << "[1] size = " << values.size() << ", sum = " << sum(values)
            << ", front = " << values.front() << ", back = " << values.back()
            << '\n';

  // 场景 2：tuple 协议按编译期下标访问，长度本身也是类型的一部分。
  std::cout << "[2] get<0> = " << std::get<0>(values)
            << ", tuple_size = " << std::tuple_size<decltype(values)>::value
            << '\n';

  // 场景 3：按值复制，并支持字典序比较。
  std::array<int, 5> copy = values;
  std::array<int, 5> larger{{3, 1, 4, 1, 6}};
  std::cout << "[3] copy == values: " << (copy == values ? "true" : "false")
            << ", values < larger: " << (values < larger ? "true" : "false")
            << '\n';

  // 场景 4：空花括号值初始化，所有元素都是 0。
  std::array<int, 5> zeros{};
  std::cout << "[4] zeros = " << zeros.front() << ' ' << zeros.back()
            << ", zero count = " << std::count(zeros.begin(), zeros.end(), 0)
            << '\n';

  // 场景 5：标准算法直接接收迭代器，原地排序。
  std::sort(values.begin(), values.end());
  std::cout << "[5] sorted = " << values[0] << ' ' << values[2] << ' '
            << values[4] << '\n';

  // 场景 6：at() 越界抛异常，operator[] 不做检查；fill 整体赋值。
  try {
    values.at(10);
  } catch (const std::out_of_range&) {
    std::cout << "[6] at(10) throws out_of_range";
  }
  values.fill(7);
  std::cout << ", after fill = " << values.front() << ' ' << values.back()
            << '\n';
}
