#include <cctype>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

template <typename T>
using Matrix = std::vector<std::vector<T>>;

template <typename Key, typename Value>
using Table = std::unordered_map<Key, Value>;

struct CiHash {
  std::size_t operator()(const std::string& key) const {
    std::size_t hash = 0;
    for (char ch : key) {
      hash = hash * 31 + static_cast<unsigned char>(
                             std::tolower(static_cast<unsigned char>(ch)));
    }
    return hash;
  }
};

struct CiEqual {
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    if (lhs.size() != rhs.size()) return false;
    for (std::size_t i = 0; i != lhs.size(); ++i) {
      if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
          std::tolower(static_cast<unsigned char>(rhs[i]))) {
        return false;
      }
    }
    return true;
  }
};

// 别名把哈希与相等策略固定在右侧，调用方只写值类型。
template <typename Value>
using CiMap = std::unordered_map<std::string, Value, CiHash, CiEqual>;

// 别名模板不能特化：先用类模板做偏特化，再用别名把 typename ::type 收短。
template <bool Condition, typename T = void>
struct enable_if_impl {};

template <typename T>
struct enable_if_impl<true, T> {
  using type = T;
};

template <bool Condition, typename T = void>
using enable_if_t = typename enable_if_impl<Condition, T>::type;

template <typename T, typename = enable_if_t<std::is_integral<T>::value>>
T twice(T value) {
  return value * 2;
}

template <typename T>
using remove_cvref_t =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template <typename... Args>
using Fn = std::function<int(Args...)>;

int main() {
  // 场景 1：别名模板就是展开后的类型，不产生新的类型身份。
  Matrix<int> grid{{1, 2}, {3, 4}};
  std::cout << "[1] rows = " << grid.size() << ", cell = " << grid[1][1]
            << ", same as vector<vector<int>> = "
            << std::is_same<Matrix<int>, std::vector<std::vector<int>>>::value
            << '\n';

  // 场景 2：别名转发模板参数，业务侧不再重复写完整类型。
  Table<std::string, int> ages;
  ages.emplace("atlas", 3);
  ages.emplace("modern", 11);
  std::cout << "[2] entries = " << ages.size()
            << ", modern = " << ages.at("modern") << '\n';

  // 场景 3：别名可以固定策略，忽略大小写的键直接命中。
  CiMap<int> scores;
  scores["Alice"] = 90;
  std::cout << "[3] find(alice) = "
            << (scores.find("alice") != scores.end() ? "hit" : "miss")
            << ", at(ALICE) = " << scores.at("ALICE") << '\n';

  // 场景 4：先特化类模板，再用别名包一层，得到 enable_if_t。
  std::cout << "[4] twice(21) = " << twice(21)
            << ", enable_if_t<true, int> is int = "
            << std::is_same<enable_if_t<true, int>, int>::value << '\n';

  // 场景 5：别名缩短 typename ...::type 链。
  std::cout << "[5] remove_cvref_t<const int&> is int = "
            << std::is_same<remove_cvref_t<const int&>, int>::value << '\n';

  // 场景 6：别名模板可以转发参数包。
  Fn<int, int> add = [](int lhs, int rhs) { return lhs + rhs; };
  std::cout << "[6] Fn<int, int>(2, 3) = " << add(2, 3) << '\n';
}
