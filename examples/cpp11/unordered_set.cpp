#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

struct Point {
  int x;
  int y;
};

struct PointHash {
  std::size_t operator()(const Point& point) const {
    return std::hash<int>()(point.x) ^ (std::hash<int>()(point.y) << 1);
  }
};

struct PointEqual {
  bool operator()(const Point& lhs, const Point& rhs) const {
    return lhs.x == rhs.x && lhs.y == rhs.y;
  }
};

int main() {
  // 场景 1：重复插入不改变大小，count 与 erase 各司其职。
  std::unordered_set<int> seen{1, 2, 3};
  auto inserted = seen.insert(2);
  seen.insert(4);
  std::cout << "[1] size = " << seen.size()
            << ", duplicate insert = " << (inserted.second ? "new" : "exists")
            << ", count(3) = " << seen.count(3)
            << ", erase(1) = " << seen.erase(1) << ", size = " << seen.size()
            << '\n';

  // 场景 2：用迭代器区间构造集合，天然完成去重。
  std::vector<int> input{5, 1, 5, 2, 1, 5};
  std::unordered_set<int> unique(input.begin(), input.end());
  std::cout << "[2] input = " << input.size() << ", unique = " << unique.size()
            << ", count(5) = " << unique.count(5) << '\n';

  // 场景 3：自定义类型通过哈希与相等函数去重，emplace 返回是否插入。
  std::unordered_set<Point, PointHash, PointEqual> points;
  const auto first = points.emplace(Point{1, 2});
  const auto duplicate = points.emplace(Point{1, 2});
  points.emplace(Point{3, 4});
  std::cout << "[3] first = " << (first.second ? "new" : "exists")
            << ", duplicate = " << (duplicate.second ? "new" : "exists")
            << ", size = " << points.size() << ", find = "
            << (points.find(Point{3, 4}) != points.end() ? "hit" : "miss")
            << '\n';

  // 场景 4：字符串集合区分大小写。
  std::unordered_set<std::string> tags{"atlas"};
  std::cout << "[4] atlas = " << tags.count("atlas")
            << ", Atlas = " << tags.count("Atlas") << '\n';
}
