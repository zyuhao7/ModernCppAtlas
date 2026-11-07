#include <cstddef>
#include <iostream>
#include <unordered_set>

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
  std::unordered_set<int> seen{1, 2, 3};
  auto inserted = seen.insert(2);
  seen.insert(4);
  std::cout << seen.size() << ' ' << (inserted.second ? "new" : "exists")
            << '\n';
  std::cout << (seen.count(3) ? "found" : "missing") << ' '
            << (seen.count(9) ? "found" : "missing") << '\n';
  seen.erase(1);
  std::cout << seen.size() << '\n';

  std::unordered_set<Point, PointHash, PointEqual> points;
  points.insert(Point{1, 2});
  points.insert(Point{1, 2});
  points.insert(Point{3, 4});
  std::cout << points.size() << ' '
            << (points.count(Point{1, 2}) ? "yes" : "no") << '\n';
}
