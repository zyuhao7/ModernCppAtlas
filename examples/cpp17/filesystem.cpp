#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main() {
  const fs::path log = "/var/log/app/server.log";
  std::cout << log.parent_path().generic_string() << '\n';
  std::cout << log.stem().string() << ' ' << log.extension().string() << '\n';

  fs::path backup = log;
  backup.replace_filename("backup.log");
  std::cout << backup.generic_string() << '\n';

  const fs::path root = fs::temp_directory_path() / "atlas-filesystem-example";
  fs::remove_all(root);
  fs::create_directories(root / "logs" / "archive");

  {
    std::ofstream(root / "logs" / "server.log") << "hello";
    std::ofstream(root / "logs" / "config.ini") << "port=8080";
  }

  std::vector<std::string> entries;
  for (const auto& entry : fs::directory_iterator(root / "logs")) {
    entries.push_back(entry.path().filename().string());
  }
  std::sort(entries.begin(), entries.end());

  std::cout << "entries:";
  for (const auto& name : entries) std::cout << ' ' << name;
  std::cout << '\n';

  std::cout << "size: " << fs::file_size(root / "logs" / "server.log") << '\n';
  std::cout << "relative: "
            << fs::relative(root / "logs" / "server.log", root).generic_string()
            << '\n';

  fs::remove_all(root);
}
