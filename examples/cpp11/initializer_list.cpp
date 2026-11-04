#include <initializer_list>
#include <iostream>
#include <numeric>
#include <vector>

class Numbers {
 public:
  Numbers(std::initializer_list<int> values) : values_(values) {}

  std::size_t size() const { return values_.size(); }

  int sum() const {
    int total = 0;
    for (int value : values_) {
      total += value;
    }
    return total;
  }

 private:
  std::vector<int> values_;
};

int total(std::initializer_list<int> values) {
  return std::accumulate(values.begin(), values.end(), 0);
}

int main() {
  Numbers numbers{1, 2, 3, 4};
  std::cout << numbers.size() << ' ' << numbers.sum() << '\n';
  std::cout << total({10, 20, 30}) << '\n';

  std::initializer_list<int> values{7, 8, 9};
  int sum = 0;
  for (int value : values) {
    sum += value;
  }
  std::cout << values.size() << ' ' << sum << '\n';
}
