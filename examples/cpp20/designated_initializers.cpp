#include <iostream>
#include <string>

struct Point {
  int x = 1;
  int y = 2;
};

struct Segment {
  Point from;
  Point to;
  bool visible = true;
};

struct Config {
  int retries = 3;
  std::string name = "atlas";
  bool debug = false;
};

int main() {
  Point p{.y = 9};
  std::cout << "[1] p = " << p.x << ' ' << p.y << '\n';

  Segment s{.from = {.x = 3}, .to = p};
  std::cout << "[2] segment = " << s.from.x << ' ' << s.from.y << ' ' << s.to.x
            << ' ' << s.to.y << ' ' << s.visible << '\n';

  auto q = new Point{.x = 4};
  std::cout << "[3] new point = " << q->x << ' ' << q->y << '\n';
  delete q;

  Config config{.debug = true};
  std::cout << "[4] config = " << config.retries << ' ' << config.name << ' '
            << config.debug << '\n';

  Segment full{
      .from = {.x = 5, .y = 6}, .to = {.x = 7, .y = 8}, .visible = false};
  std::cout << "[5] full = " << full.from.x << ' ' << full.from.y << ' '
            << full.to.x << ' ' << full.to.y << ' ' << full.visible << '\n';
}
