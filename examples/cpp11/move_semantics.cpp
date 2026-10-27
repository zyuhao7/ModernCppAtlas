#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Buffer {
 public:
  explicit Buffer(std::vector<int> values) : values_(std::move(values)) {}

  std::size_t size() const { return values_.size(); }

 private:
  std::vector<int> values_;
};

int main() {
  std::vector<int> source{1, 2, 3};
  Buffer buffer(std::move(source));

  std::string text = "atlas";
  std::string moved = std::move(text);

  std::cout << buffer.size() << ' ' << moved << '\n';
}
