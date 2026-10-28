#include <iostream>

class NonCopyable {
 public:
  NonCopyable() = default;
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
  virtual ~NonCopyable() = default;
};

class Point {
 public:
  Point() = default;
  Point(int x, int y) : x_(x), y_(y) {}
  Point(const Point&) = default;
  Point& operator=(const Point&) = default;

  int x() const { return x_; }
  int y() const { return y_; }

 private:
  int x_ = 0;
  int y_ = 0;
};

int main() {
  Point a(1, 2);
  Point b = a;
  NonCopyable n;
  (void)n;

  std::cout << b.x() << ' ' << b.y() << '\n';
}
