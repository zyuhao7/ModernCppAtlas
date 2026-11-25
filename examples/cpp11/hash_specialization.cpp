#include <cstddef>
#include <iostream>
#include <unordered_set>

struct Point {
  int x;
  int y;

  bool operator==(const Point& other) const {
    return x == other.x && y == other.y;
  }
};

namespace std {
template <>
struct hash<Point> {
  size_t operator()(const Point& p) const noexcept {
    return hash<int>()(p.x) * 31u + hash<int>()(p.y);
  }
};
}  // namespace std

int main() {
  const std::hash<Point> point_hash;
  std::cout << "[1] hash(Point{1, 2}) stable = "
            << (point_hash(Point{1, 2}) == point_hash(Point{1, 2})) << '\n';

  std::unordered_set<Point> points;
  points.insert(Point{1, 2});
  points.insert(Point{1, 2});
  points.insert(Point{3, 4});
  std::cout << "[2] unordered_set size after 3 inserts = " << points.size()
            << '\n';

  std::cout << "[3] contains(Point{3, 4}) = " << points.count(Point{3, 4})
            << ", contains(Point{5, 6}) = " << points.count(Point{5, 6})
            << '\n';
}
