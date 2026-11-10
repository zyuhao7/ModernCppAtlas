#include <iostream>
#include <string>

class Base {
 public:
  explicit Base(int value) { std::cout << "int:" << value << '\n'; }

  explicit Base(const std::string& label) {
    std::cout << "str:" << label << '\n';
  }
};

class Derived : public Base {
 public:
  using Base::Base;
};

int main() {
  Derived first(7);
  Derived second(std::string("atlas"));
}
