#include <iostream>
#include <map>
#include <mutex>
#include <string>

int main() {
  std::map<std::string, int> counts{{"apple", 2}};

  if (auto it = counts.find("apple"); it != counts.end()) {
    std::cout << "[1] apple = " << it->second << '\n';
  }

  if (auto [it, inserted] = counts.try_emplace("pear", 5); inserted) {
    std::cout << "[2] pear = " << it->second << ", inserted = " << inserted
              << '\n';
  }
  if (auto [it, inserted] = counts.try_emplace("pear", 9); !inserted) {
    std::cout << "[2] pear again = " << it->second
              << ", inserted = " << inserted << '\n';
  }

  std::mutex mu;
  int pending = 2;
  if (std::lock_guard lock(mu); pending > 0) {
    std::cout << "[3] pending = " << pending << '\n';
  }

  switch (int code = 3; code) {
    case 3:
      std::cout << "[4] three\n";
      break;
    default:
      break;
  }

  if (auto it = counts.find("missing"); it == counts.end()) {
    std::cout << "[5] missing = 1\n";
  }
}
