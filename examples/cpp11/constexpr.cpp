#include <array>
#include <iostream>

// C++11 的 constexpr 函数体只能有一条 return，用三元表达式和递归表达分支。
constexpr int square(int x) { return x * x; }

constexpr int sum_squares(int n) {
  return n == 0 ? 0 : square(n) + sum_squares(n - 1);
}

// 编译期求得 14，可直接作为数组维度。
constexpr int kSize = sum_squares(3);
constexpr int kSquareOfThree = square(3);

int main() {
  // 场景 1：constexpr 结果用于 static_assert 与 std::array 长度，
  // 两者都在编译期检查，运行期不产生计算。
  static_assert(square(4) == 16, "computed at compile time");
  std::array<int, kSize> table{};
  std::cout << "[1] kSize = " << kSize << ", array size = " << table.size()
            << '\n';

  // 场景 2：同一个函数用运行期实参调用时就是普通函数，
  // 不能要求它在编译期完成计算。
  int runtime_input = 5;
  std::cout << "[2] square(5) at runtime = " << square(runtime_input) << '\n';

  // 场景 3：常量表达式也可以直接写在模板实参位置，
  // std::array<int, square(2)> 的长度在编译期确定为 4。
  std::cout << "[3] square(3) as constant = " << kSquareOfThree
            << ", template size = " << std::array<int, square(2)>().size()
            << '\n';
}
