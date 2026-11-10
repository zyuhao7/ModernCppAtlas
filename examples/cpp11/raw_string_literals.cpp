#include <iostream>
#include <string>

int main() {
  std::string pattern = R"(\d+\.\d+)";
  std::string json = R"({"name":"atlas","size":3})";
  std::cout << pattern << '\n';
  std::cout << json << '\n';
}
