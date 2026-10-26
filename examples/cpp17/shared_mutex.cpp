#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class Registry {
 public:
  std::string get_or_create(const std::string &key) {
    {
      std::shared_lock lock(mu_);
      if (auto it = values_.find(key); it != values_.end()) {
        return it->second;
      }
    }

    std::unique_lock lock(mu_);
    auto it = values_.try_emplace(key, key + "-value").first;
    return it->second;
  }

 private:
  mutable std::shared_mutex mu_;
  std::unordered_map<std::string, std::string> values_;
};

int main() {
  Registry registry;
  std::cout << registry.get_or_create("alpha") << '\n';
  std::cout << registry.get_or_create("alpha") << '\n';
}
