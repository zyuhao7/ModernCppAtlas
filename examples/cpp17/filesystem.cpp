#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main() {
  fs::path p = "/var/log/app/server.log";
  std::cout << p.parent_path().string() << '\n';
  std::cout << p.stem().string() << '\n';
  std::cout << p.extension().string() << '\n';
  std::cout << (p.parent_path() / "backup.log").generic_string() << '\n';
}
