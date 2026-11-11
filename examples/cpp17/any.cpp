#include <any>
#include <cstddef>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

struct User {
  std::string name;
  int level;
};

void print(const std::any& value) {
  if (!value.has_value()) {
    std::cout << "empty";
    return;
  }
  if (value.type() == typeid(int)) {
    std::cout << "int " << std::any_cast<int>(value);
  } else if (value.type() == typeid(std::string)) {
    std::cout << "string " << std::any_cast<const std::string&>(value);
  } else if (value.type() == typeid(User)) {
    const User& user = std::any_cast<const User&>(value);
    std::cout << "user " << user.name << ' ' << user.level;
  } else {
    std::cout << "unknown";
  }
}

int main() {
  std::any value = 42;
  std::cout << "[1] count = ";
  print(value);
  std::cout << '\n';

  value = std::string("atlas");
  std::cout << "[2] name = ";
  print(value);
  std::cout << '\n';
  std::cout << "[2] reference cast = "
            << std::any_cast<const std::string&>(value) << '\n';

  std::vector<std::any> values{User{"amy", 3}, 7, std::string("x")};
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << "[3] item " << i << " = ";
    print(values[i]);
    std::cout << '\n';
  }

  value.reset();
  std::cout << "[4] has_value = " << value.has_value() << '\n';
  try {
    (void)std::any_cast<int>(value);
  } catch (const std::bad_any_cast&) {
    std::cout << "[4] cast = bad_any_cast\n";
  }

  int* number = std::any_cast<int>(&values[1]);
  std::cout << "[5] pointer cast = " << *number << '\n';
  std::cout << "[5] wrong pointer cast = "
            << (std::any_cast<std::string>(&values[1]) == nullptr) << '\n';

  const std::any text = "atlas";
  std::cout << "[6] stored type is const char* = "
            << (text.type() == typeid(const char*)) << '\n';
  std::cout << "[6] cast to string = "
            << (std::any_cast<std::string>(&text) == nullptr) << '\n';
}
