#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {

fs::path make_unique_temp_dir() {
  std::random_device rd;
  for (int attempt = 0; attempt < 32; ++attempt) {
    fs::path candidate = fs::temp_directory_path() /
                         ("atlas-filesystem-example-" + std::to_string(rd()) +
                          "-" + std::to_string(rd()));
    if (fs::create_directory(candidate)) {
      return candidate;
    }
  }
  throw std::runtime_error("could not create a unique temporary directory");
}

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
  std::cout << log.parent_path().generic_string() << '\n';
  std::cout << log.stem().string() << ' ' << log.extension().string() << '\n';

  fs::path backup = log;
  backup.replace_filename("backup.log");
  std::cout << backup.generic_string() << '\n';

  TempDir temp{make_unique_temp_dir()};
  const fs::path& root = temp.path;
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
}
