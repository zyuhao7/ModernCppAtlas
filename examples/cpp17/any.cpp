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

void print(const std::string &key, const std::any &value) {
  if (!value.has_value()) {
    std::cout << key << ": empty\n";
    return;
  }
  if (value.type() == typeid(int)) {
    std::cout << key << ": int " << std::any_cast<int>(value) << '\n';
  } else if (value.type() == typeid(std::string)) {
    std::cout << key << ": string " << std::any_cast<const std::string &>(value)
              << '\n';
  } else if (value.type() == typeid(User)) {
    const User &user = std::any_cast<const User &>(value);
    std::cout << key << ": user " << user.name << ' ' << user.level << '\n';
  } else {
    std::cout << key << ": unknown\n";
  }
}

int main() {
  std::any value = 42;
  print("count", value);

  value = std::string("atlas");
  print("name", value);

  std::vector<std::any> values{User{"amy", 3}, 7, std::string("x")};
  for (std::size_t i = 0; i < values.size(); ++i) print("item", values[i]);

  value.reset();
  print("after reset", value);

  const std::any &first = values[0];
  std::cout << std::any_cast<const User &>(first).name << '\n';

  int *number = std::any_cast<int>(&values[1]);
  std::cout << "pointer cast " << *number << '\n';

  try {
    std::cout << std::any_cast<int>(first) << '\n';
  } catch (const std::bad_any_cast &) {
    std::cout << "bad_any_cast\n";
  }
}
