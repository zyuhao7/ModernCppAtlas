#include <iostream>

struct Point {
  int x = 1;
  int y = 2;
};

struct Segment {
  Point from;
  Point to;
  bool visible = true;
};

int main() {
  Point p{.y = 9};
  Segment s{.from = {.x = 3}, .to = p};
  auto q = new Point{.x = 4};

  std::cout << p.x << ' ' << p.y << ' ' << s.from.x << ' ' << s.to.y << ' '
            << s.visible << ' ' << q->x << '\n';
  delete q;
}
