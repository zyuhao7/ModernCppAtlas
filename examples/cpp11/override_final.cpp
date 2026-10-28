#include <iostream>
#include <memory>
#include <vector>

class Shape {
 public:
  virtual ~Shape() = default;
  virtual double area() const = 0;
};

class Square final : public Shape {
 public:
  explicit Square(double side) : side_(side) {}

  double area() const override { return side_ * side_; }

 private:
  double side_;
};

int main() {
  std::vector<std::unique_ptr<Shape>> shapes;
  shapes.push_back(std::unique_ptr<Shape>(new Square(3.0)));
  std::cout << shapes.front()->area() << '\n';
}
