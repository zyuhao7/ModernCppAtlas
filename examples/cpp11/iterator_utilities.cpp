#include <iostream>
#include <iterator>
#include <list>
#include <vector>

template <typename Iter>
int sum(Iter first, Iter last) {
  int total = 0;
  for (; first != last; ++first) total += *first;
  return total;
}

int main() {
  int data[] = {1, 2, 3, 4};
  std::vector<int> values{1, 2, 3, 4};

  // Non-member begin/end work for both arrays and containers.
  std::cout << "[1] array sum = " << sum(std::begin(data), std::end(data))
            << '\n';
  std::cout << "[2] vector sum = " << sum(std::begin(values), std::end(values))
            << '\n';

  // std::next returns a new iterator and leaves the original in place.
  auto it = values.begin();
  auto second = std::next(it);
  std::cout << "[3] next(begin) = " << *second << ", begin = " << *it << '\n';

  // std::prev is the counterpart; it cannot move before begin.
  std::cout << "[4] prev(end) = " << *std::prev(values.end()) << '\n';
  std::cout << "[5] distance(begin, end) = "
            << std::distance(values.begin(), values.end()) << '\n';

  // std::next also accepts a step count; distance reports the consumed range.
  std::list<int> chain{1, 2, 3};
  auto third = std::next(chain.begin(), 2);
  std::cout << "[6] list next(begin, 2) = " << *third
            << ", distance = " << std::distance(chain.begin(), third) << '\n';
}
