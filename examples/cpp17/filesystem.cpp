#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct TempDir {
  fs::path path;

  ~TempDir() {
    std::error_code ec;
    fs::remove_all(path, ec);
  }
};

}  // namespace

int main() {
  const fs::path log = "/var/log/app/server.log";

  std::cout << "[1] parent = " << log.parent_path().generic_string()
            << ", stem = " << log.stem().string()
            << ", extension = " << log.extension().string() << '\n';

  fs::path backup = log;
  backup.replace_filename("backup.log");
  std::cout << "[1] replaced = " << backup.generic_string() << '\n';

  const fs::path root =
      fs::temp_directory_path() / "moderncppatlas-cpp17-filesystem";
  std::error_code ec;
  fs::remove_all(root, ec);
  fs::create_directories(root / "logs" / "archive");
  TempDir temp{root};

  {
    std::ofstream(root / "logs" / "server.log") << "hello";
    std::ofstream(root / "logs" / "config.ini") << "port=8080";
  }

  std::vector<std::string> entries;
  for (const auto& entry : fs::directory_iterator(root / "logs")) {
    entries.push_back(entry.path().filename().string());
  }
  std::sort(entries.begin(), entries.end());

  std::cout << "[2] entries =";
  for (const auto& name : entries) {
    std::cout << ' ' << name;
  }
  std::cout << '\n';
  std::cout << "[2] archive = " << fs::is_directory(root / "logs" / "archive")
            << '\n';

  std::cout << "[3] size = " << fs::file_size(root / "logs" / "server.log")
            << ", relative = "
            << fs::relative(root / "logs" / "server.log", root).generic_string()
            << '\n';

  std::cout << "[4] server regular = "
            << fs::is_regular_file(root / "logs" / "server.log")
            << ", logs directory = " << fs::is_directory(root / "logs")
            << ", missing = " << fs::exists(root / "logs" / "missing.txt")
            << '\n';

  std::cout << "[5] normalized = "
            << fs::path("a/b/../c").lexically_normal().generic_string() << '\n';
}
