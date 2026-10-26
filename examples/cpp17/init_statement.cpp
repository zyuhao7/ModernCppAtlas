#include <iostream>
#include <map>
#include <mutex>
#include <string>

int main() {
  std::map<std::string, int> counts{{"apple", 2}};

  if (auto it = counts.find("apple"); it != counts.end()) {
    std::cout << it->second << '\n';
  }

  if (auto [it, inserted] = counts.try_emplace("pear", 5); inserted) {
    std::cout << "inserted " << it->first << '=' << it->second << '\n';
  }

  std::mutex mu;
  int pending = 2;
  if (std::lock_guard lock(mu); pending > 0) {
    std::cout << "pending " << pending << '\n';
  }

  switch (int code = 3; code) {
    case 3:
      std::cout << "three\n";
      break;
    default:
      break;
  }
}
