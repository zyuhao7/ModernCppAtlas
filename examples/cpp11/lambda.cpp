#include <algorithm>
#include <iostream>
#include <vector>

int main() {
  // 场景 1：无捕获 lambda 直接作为排序谓词，count_if 统计满足条件的元素。
  std::vector<int> values{3, 1, 4, 1, 5};
  std::sort(values.begin(), values.end(), [](int a, int b) { return a > b; });

  int threshold = 2;
  auto above = [threshold](int value) { return value > threshold; };
  long total = 0;
  std::for_each(values.begin(), values.end(),
                [&total](int value) { total += value; });

  const auto count = std::count_if(values.begin(), values.end(), above);
  std::cout << "[1] count = " << count << ", total = " << total << '\n';

  // 场景 2：按值捕获保存创建时的副本，按引用捕获看到后续修改。
  // 捕获发生在 lambda 创建时，两种方式在 threshold 改变后结果不同。
  auto by_value = [threshold] { return threshold; };
  auto by_reference = [&threshold] { return threshold; };
  threshold = 10;
  std::cout << "[2] by_value = " << by_value()
            << ", by_reference = " << by_reference() << '\n';

  // 场景 3：mutable 允许修改闭包自己的副本，外部变量不受影响。
  // 每调用一次 tick，闭包内部计数加一，而 outer 始终为 0。
  int outer = 0;
  auto tick = [outer]() mutable { return ++outer; };
  tick();
  tick();
  std::cout << "[3] closure count = " << tick() << ", outer count = " << outer
            << '\n';

  // 场景 4：没有捕获的 lambda 可以转换为函数指针，但有捕获时不行。
  int (*square)(int) = [](int value) { return value * value; };
  std::cout << "[4] function pointer = " << square(6) << '\n';
}
