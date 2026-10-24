#include <iostream>
#include <map>
#include <string>
#include <tuple>

std::tuple<int, int> divmod(int value, int divisor) {
  return {value / divisor, value % divisor};
}

std::tuple<std::string, int, bool> make_user() {
  return {"amy", 30, true};
}

struct Point {
  int x;
  int y;
};

int main() {
  auto [quotient, remainder] = divmod(17, 5);
  std::cout << quotient << ' ' << remainder << '\n';

  std::map<std::string, int> counts;
  auto [it, inserted] = counts.insert({"apple", 1});
  std::cout << it->first << ' ' << inserted << '\n';

  auto [it2, inserted2] = counts.insert({"apple", 2});
  std::cout << it2->second << ' ' << inserted2 << '\n';

  auto [name, age, active] = make_user();
  std::cout << name << ' ' << age << ' ' << active << '\n';

  Point point{1, 2};
  auto& [x, y] = point;
  x += 10;
  y += 20;
  std::cout << point.x << ' ' << point.y << '\n';

  const std::map<std::string, int> scores{{"bob", 90}, {"amy", 95}};
  bool first = true;
  for (const auto& [student, score] : scores) {
    if (!first) std::cout << ' ';
    first = false;
    std::cout << student << ':' << score;
  }
  std::cout << '\n';

  int values[3] = {3, 1, 2};
  auto [low, middle, high] = values;
  std::cout << low << middle << high << '\n';
}
