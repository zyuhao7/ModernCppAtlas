#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

int main() {
  auto numbers = std::views::iota(1, 6);
#if defined(__cpp_lib_ranges_to_container) && \
    __cpp_lib_ranges_to_container >= 202202L
  auto squares = numbers | std::views::transform([](int n) { return n * n; }) |
                 std::ranges::to<std::vector<int>>();
#else
  std::vector<int> squares;
  for (int n : numbers) squares.push_back(n * n);
#endif

  std::cout << "[1] squares =";
  for (int n : squares) std::cout << ' ' << n;
  std::cout << '\n';

  std::string_view csv = "alpha,beta,gamma";
  auto fields =
      csv | std::views::split(',') | std::views::transform([](auto part) {
        return std::string(part.begin(), part.end());
      });
#if defined(__cpp_lib_ranges_to_container) && \
    __cpp_lib_ranges_to_container >= 202202L
  auto words = fields | std::ranges::to<std::vector<std::string>>();
#else
  std::vector<std::string> words;
  for (const auto &word : fields) words.push_back(word);
#endif

  std::cout << "[2] fields = " << words.size() << '\n';
  std::cout << "[3] first = " << words.front() << '\n';
}
