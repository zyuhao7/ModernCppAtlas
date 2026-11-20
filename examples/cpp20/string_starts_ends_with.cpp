#include <iostream>
#include <string>
#include <string_view>

int main() {
  const std::string path = "/usr/local/bin/tool";
  const std::string_view name = "archive.tar.gz";

  std::cout << "[1] absolute = " << path.starts_with('/') << '\n';
  std::cout << "[2] tool = " << path.ends_with("tool")
            << ", l = " << path.ends_with('l') << '\n';
  std::cout << "[3] gz = " << name.ends_with(".gz")
            << ", bz2 = " << name.ends_with(".bz2") << '\n';
  std::cout << "[4] archive = " << name.starts_with("archive")
            << ", tar = " << name.starts_with("tar") << '\n';
  std::cout << "[5] empty = " << path.starts_with("") << ", "
            << path.ends_with("") << '\n';
  std::cout << "[6] case = " << path.starts_with("/USR") << '\n';
}
