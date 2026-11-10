#include <iostream>
#include <string>
#include <utility>

class Logger {
 public:
  Logger() : Logger("info") {}

  explicit Logger(std::string level) : level_(std::move(level)) {
    std::cout << "start " << level_ << '\n';
  }

 private:
  std::string level_;
};

int main() {
  Logger fallback;
  Logger custom("debug");
}
