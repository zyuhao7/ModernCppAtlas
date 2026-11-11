#include <chrono>
#include <iostream>
#include <thread>

int main() {
  using namespace std::chrono;

  // 场景 1：duration 在类型里携带单位。毫秒转秒必须显式
  // duration_cast，小数部分被截断；转 duration<double> 保留比例。
  milliseconds interval{1500};
  seconds rounded = duration_cast<seconds>(interval);
  duration<double> as_seconds = interval;
  std::cout << "[1] ms = " << interval.count()
            << ", cast to s = " << rounded.count()
            << ", as double s = " << as_seconds.count() << '\n';

  // 场景 2：扩大单位可以隐式转换，minutes 赋给 seconds 无需 cast。
  minutes minute{1};
  seconds minute_as_seconds = minute;
  std::cout << "[2] minute in s = " << minute_as_seconds.count()
            << ", in ms = " << duration_cast<milliseconds>(minute).count()
            << '\n';

  // 场景 3：两个同单位时长相除得到无量纲标量；整数计数会截断，
  // 因此先把计数换成 double 再相除。
  double ratio = duration<double>(milliseconds(750)) / seconds(1);
  std::cout << "[3] 750ms / 1s = " << ratio << '\n';

  // 场景 4：steady_clock 单调递增，适合测耗时；两个时间点相减
  // 得到 duration，再转成目标单位。
  steady_clock::time_point start = steady_clock::now();
  std::this_thread::sleep_for(milliseconds(2));
  steady_clock::time_point end = steady_clock::now();
  auto elapsed = duration_cast<microseconds>(end - start);
  std::cout << "[4] slept at least 2ms = " << std::boolalpha
            << (elapsed >= milliseconds(2)) << '\n';

  // 场景 5：time_point 加 duration 得到截止时刻，可直接比较。
  auto deadline = start + seconds(1);
  std::cout << "[5] deadline after end = " << (deadline > end)
            << ", remaining s = "
            << duration_cast<seconds>(deadline - end).count() << '\n';
}
