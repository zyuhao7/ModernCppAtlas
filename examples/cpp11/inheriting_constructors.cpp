#include <iostream>
#include <string>
#include <type_traits>

class Base {
 public:
  explicit Base(int value) : value_(value) {
    std::cout << "base int:" << value_ << '\n';
  }

  explicit Base(const std::string& label) : label_(label) {
    std::cout << "base str:" << label_ << '\n';
  }

  int value() const { return value_; }
  const std::string& label() const { return label_; }

 private:
  int value_ = 0;
  std::string label_;
};

class Derived : public Base {
 public:
  using Base::Base;

  int extra() const { return extra_; }

 private:
  int extra_ = 5;
};

class Tagged : public Base {
 public:
  using Base::Base;

  Tagged() : Base(0) { std::cout << "tagged default body" << '\n'; }
};

int main() {
  // 场景 1：两个基类构造函数重载都被继承。
  Derived first(7);
  Derived second(std::string("atlas"));
  std::cout << "[1] first.value = " << first.value()
            << ", second.label = " << second.label() << '\n';

  // 场景 2：继承构造函数只初始化基类，派生类的成员初始化器照常执行。
  std::cout << "[2] extra after inherited ctor = " << first.extra() << '\n';

  // 场景 3：继承构造函数不影响隐式生成的拷贝构造。
  Derived copy = first;
  std::cout << "[3] copied value = " << copy.value()
            << ", extra = " << copy.extra() << '\n';

  // 场景 4：派生类可以同时拥有自己的构造函数和继承来的构造函数。
  Tagged tagged_default;
  Tagged tagged_inherited(3);
  std::cout << "[4] default value = " << tagged_default.value()
            << ", inherited value = " << tagged_inherited.value() << '\n';

  // 场景 5：继承效果可以直接用类型萃取观察。
  std::cout << "[5] constructible from int = "
            << std::is_constructible<Derived, int>::value << ", from string = "
            << std::is_constructible<Derived, std::string>::value << '\n';
}
