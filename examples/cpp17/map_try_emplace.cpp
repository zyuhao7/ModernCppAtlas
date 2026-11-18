#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

struct Tracked {
  static int constructions;
  std::string value;

  explicit Tracked(std::string v) : value(std::move(v)) { ++constructions; }
};

int Tracked::constructions = 0;

int main() {
  std::map<std::string, Tracked> entries;

  const auto [first_it, first_ok] = entries.try_emplace("a", "first");
  const auto [second_it, second_ok] = entries.try_emplace("a", "second");
  std::cout << "[1] inserted = " << first_ok << ", exists = " << second_ok
            << '\n';
  std::cout << "[2] value = " << first_it->second.value << '\n';
  std::cout << "[3] constructions = " << Tracked::constructions << '\n';

  entries.insert_or_assign("a", Tracked("third"));
  std::cout << "[4] after assign = " << entries.at("a").value << '\n';
  std::cout << "[5] constructions = " << Tracked::constructions << '\n';

  std::unordered_map<int, std::string> numbers;
  numbers.try_emplace(1, "one");
  numbers.try_emplace(1, "uno");
  std::cout << "[6] size = " << numbers.size() << ", value = " << numbers.at(1)
            << '\n';
}
