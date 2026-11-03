#include <chrono>
#include <iostream>
#include <thread>

int main() {
  using namespace std::chrono;

  milliseconds interval{1500};
  seconds rounded = duration_cast<seconds>(interval);
  std::cout << interval.count() << ' ' << rounded.count() << '\n';

  duration<double> as_seconds = interval;
  std::cout << as_seconds.count() << '\n';

  minutes minute{1};
  std::cout << duration_cast<milliseconds>(minute).count() << '\n';

  steady_clock::time_point start = steady_clock::now();
  std::this_thread::sleep_for(milliseconds(2));
  steady_clock::time_point end = steady_clock::now();
  const auto elapsed = duration_cast<milliseconds>(end - start);
  std::cout << (elapsed >= milliseconds::zero()) << '\n';

  const auto deadline = start + seconds(1);
  std::cout << (deadline > end) << '\n';
}
