#include <iostream>
#include <map>
#include <string>
#include <tuple>

std::tuple<int, int> divmod(int value, int divisor) {
  return {value / divisor, value % divisor};
}

std::tuple<std::string, int, bool> make_user() { return {"amy", 30, true}; }

struct Point {
  int x;
  int y;
};

int main() {
  // tuple 返回值的两个位置直接变成具名变量。
  const auto [quotient, remainder] = divmod(17, 5);
  std::cout << "[1] divmod(17, 5) = " << quotient << " remainder " << remainder
            << '\n';

  std::map<std::string, int> counts;
  const auto [it, inserted] = counts.insert({"apple", 1});
  const auto [existing, duplicate] = counts.insert({"apple", 2});
  std::cout << "[2] inserted = " << inserted << ", key = " << it->first
            << ", duplicate = " << duplicate
            << ", kept value = " << existing->second << '\n';

  const auto [name, age, active] = make_user();
  std::cout << "[3] user = " << name << ", age = " << age
            << ", active = " << active << '\n';

  // 按值绑定得到副本，按引用绑定才会改回原对象。
  Point point{1, 2};
  auto [copy_x, copy_y] = point;
  copy_x = 100;
  auto& [x, y] = point;
  x += 10;
  y += 20;
  std::cout << "[4] copy = " << copy_x << ' ' << copy_y
            << ", point = " << point.x << ' ' << point.y << '\n';

  const std::map<std::string, int> scores{{"bob", 90}, {"amy", 95}};
  std::cout << "[5] scores =";
  for (const auto& [student, score] : scores) {
    std::cout << ' ' << student << ':' << score;
  }
  std::cout << '\n';

  int values[3] = {3, 1, 2};
  const auto [low, middle, high] = values;
  std::cout << "[6] array = " << low << ' ' << middle << ' ' << high << '\n';
}
