#include <iostream>
#include <memory>
#include <vector>

class Shape {
 public:
  virtual ~Shape() = default;
  virtual double area() const = 0;
  virtual const char* name() const = 0;
};

// 场景 1：final 类不能再被继承，override 让签名错误在编译期暴露。
class Square final : public Shape {
 public:
  explicit Square(double side) : side_(side) {}

  double area() const override { return side_ * side_; }
  const char* name() const override { return "square"; }

 private:
  double side_;
};

class Circle : public Shape {
 public:
  explicit Circle(double radius) : radius_(radius) {}

  double area() const override { return 3.14159 * radius_ * radius_; }
  const char* name() const override { return "circle"; }

 private:
  double radius_;
};

// 场景 2：final 只加在虚函数上，类本身仍可被继承。
class Base {
 public:
  virtual ~Base() = default;
  virtual int id() const { return 1; }
  virtual const char* tag() const final { return "base"; }
};

class Derived : public Base {
 public:
  int id() const override { return 2; }
  // const char* tag() const override;  // 编译失败：Base::tag 已 final
};

// 场景 3：override 也适用于虚析构函数，签名不符即报错。
struct Resource {
  virtual ~Resource() = default;
  virtual void release() {}
};

struct Tracked : Resource {
  ~Tracked() override { std::cout << "[3] tracked destroyed\n"; }
  void release() override { std::cout << "[3] tracked released\n"; }
};

int main() {
  // 场景 1：通过基类指针调用，虚表分派到派生实现。
  std::vector<std::unique_ptr<Shape>> shapes;
  shapes.push_back(std::unique_ptr<Shape>(new Square(3.0)));
  shapes.push_back(std::unique_ptr<Shape>(new Circle(1.0)));
  std::cout << "[1]";
  for (const auto& shape : shapes) {
    std::cout << ' ' << shape->name() << " area = " << shape->area();
  }
  std::cout << '\n';

  // 场景 2：id 可继续覆写，tag 被 final 封死。
  Derived derived;
  Base base;
  std::cout << "[2] derived id = " << derived.id()
            << ", derived tag = " << derived.tag()
            << ", base id = " << base.id() << '\n';

  // 场景 3：override 修饰虚析构，多态删除时派生析构被调用。
  {
    std::unique_ptr<Resource> resource(new Tracked());
    resource->release();
  }

  // 场景 4：final 不阻止组合，只阻止继承。
  struct Wrapper {
    Square square{2.0};
  };
  Wrapper wrapper;
  std::cout << "[4] composed square area = " << wrapper.square.area() << '\n';
}
