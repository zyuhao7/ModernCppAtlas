#include <algorithm>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

template <std::ranges::input_range R>
void print(R range) {
  bool first = true;
  for (const auto& value : range) {
    if (!first) {
      std::cout << ' ';
    }
    first = false;
    std::cout << value;
  }
  std::cout << '\n';
}

int main() {
  std::vector<int> values{5, 1, 4, 2, 3};
  std::ranges::sort(values);

  const auto scaled_odds =
      values | std::views::filter([](int value) { return value % 2 == 1; }) |
      std::views::transform([](int value) { return value * 10; });
  std::cout << "[1] scaled odds = ";
  print(scaled_odds);

  std::cout << "[2] last two = ";
  print(values | std::views::reverse | std::views::take(2));

  const auto multiples =
      std::views::iota(1, 101) |
      std::views::filter([](int value) { return value % 7 == 0; }) |
      std::views::take(3);
  std::cout << "[3] multiples of 7 = ";
  print(multiples);

  struct Task {
    std::string name;
    int priority;
  };

  std::vector<Task> tasks{{"build", 2}, {"test", 1}, {"deploy", 3}};
  std::ranges::sort(tasks, {}, &Task::priority);

  std::cout << "[4] tasks by priority = ";
  for (std::size_t i = 0; i < tasks.size(); ++i) {
    if (i != 0) {
      std::cout << ' ';
    }
    std::cout << tasks[i].name;
  }
  std::cout << '\n';

  std::cout << "[5] top priorities = ";
  print(tasks | std::views::reverse | std::views::take(2) |
        std::views::transform(&Task::name));

  const auto odd_count =
      std::ranges::count_if(values, [](int value) { return value % 2 == 1; });
  const auto first_odd =
      *std::ranges::find_if(values, [](int value) { return value % 2 == 1; });
  std::cout << "[6] odd count = " << odd_count << ", first odd = " << first_odd
            << '\n';
}
