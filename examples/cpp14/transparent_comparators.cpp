#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>

struct Name {
  static int constructions;

  std::string text;

  Name(const char* s) : text(s) { ++constructions; }
  explicit Name(std::string s) : text(std::move(s)) { ++constructions; }

  bool operator<(const Name& other) const { return text < other.text; }
  bool operator<(const char* other) const { return text < other; }
  bool operator<(const std::string& other) const { return text < other; }
};

int Name::constructions = 0;

bool operator<(const char* lhs, const Name& rhs) { return lhs < rhs.text; }

int main() {
  // std::less<> 是透明比较器：find 接收 const char*，不必先构造 Name。
  std::map<Name, int, std::less<>> transparent;
  transparent.emplace(Name("alice"), 1);
  transparent.emplace(Name("bob"), 2);
  transparent.emplace(Name("carol"), 3);

  // std::less<Name> 要求实参先转换成 Name，异构查找会产生临时对象。
  std::map<Name, int, std::less<Name>> classic(transparent.begin(),
                                               transparent.end());

  Name::constructions = 0;
  const bool transparent_hit = transparent.find("bob") != transparent.end();
  std::cout << "[1] transparent find(\"bob\") found = " << transparent_hit
            << '\n';
  std::cout << "[2] Name constructions in transparent lookup = "
            << Name::constructions << '\n';

  Name::constructions = 0;
  const bool classic_hit = classic.find("dave") != classic.end();
  std::cout << "[3] classic find(\"dave\") found = " << classic_hit << '\n';
  std::cout << "[4] Name constructions in classic lookup = "
            << Name::constructions << '\n';

  // std::greater<> 同样透明：容器元素类型由模板实参推导。
  std::set<int, std::greater<>> descending{1, 2, 3};
  std::cout << "[5] descending traversal =";
  for (int value : descending) std::cout << ' ' << value;
  std::cout << '\n';
}
