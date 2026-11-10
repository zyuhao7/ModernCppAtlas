#include <iostream>
#include <string>
#include <vector>

struct Point {
  int x;
  int y;
};

int main() {
  Point origin{};
  Point corner{3, 4};
  std::vector<int> values{1, 2, 3};
  std::string label{"atlas"};

  std::cout << origin.x << ' ' << origin.y << '\n';
  std::cout << corner.x << ' ' << corner.y << '\n';
  std::cout << values.size() << ' ' << values.front() << ' ' << values.back()
            << '\n';
  std::cout << label << '\n';
}
