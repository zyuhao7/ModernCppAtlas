#include <algorithm>
#include <atomic>
#include <iostream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

class Registry {
 public:
  std::optional<std::string> get(const std::string& key) const {
    std::shared_lock lock(mu_);
    if (auto it = values_.find(key); it != values_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  // 共享锁快速命中，未命中再升级为独占锁并二次检查。
  std::string get_or_create(const std::string& key) {
    {
      std::shared_lock lock(mu_);
      if (auto it = values_.find(key); it != values_.end()) {
        return it->second;
      }
    }
    std::unique_lock lock(mu_);
    return values_.try_emplace(key, key + "-value").first->second;
  }

  bool set(const std::string& key, std::string value) {
    std::unique_lock lock(mu_);
    const bool existed = values_.find(key) != values_.end();
    values_[key] = std::move(value);
    return existed;
  }

  std::vector<std::string> keys() const {
    std::shared_lock lock(mu_);
    std::vector<std::string> result;
    result.reserve(values_.size());
    for (const auto& entry : values_) {
      result.push_back(entry.first);
    }
    std::sort(result.begin(), result.end());
    return result;
  }

 private:
  mutable std::shared_mutex mu_;
  std::unordered_map<std::string, std::string> values_;
};

int main() {
  Registry registry;

  const auto before = registry.get("alpha");
  const std::string first = registry.get_or_create("alpha");
  const std::string second = registry.get_or_create("alpha");
  std::cout << "[1] before = " << before.value_or("none")
            << ", first = " << first << ", second = " << second << '\n';

  // 多个读者只持有共享锁；汇合后再统计，输出不依赖调度顺序。
  std::atomic<int> hits{0};
  std::vector<std::thread> readers;
  for (int i = 0; i < 8; ++i) {
    readers.emplace_back([&registry, &hits] {
      if (registry.get("alpha")) {
        hits.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }
  for (std::thread& reader : readers) {
    reader.join();
  }
  std::cout << "[2] readers = 8, hits = " << hits.load()
            << ", distinct = " << registry.keys().size() << '\n';

  const bool existed = registry.set("alpha", "alpha-v2");
  std::cout << "[3] existed = " << existed
            << ", updated = " << registry.get("alpha").value_or("none") << '\n';

  registry.get_or_create("beta");
  registry.get_or_create("gamma");
  const std::vector<std::string> keys = registry.keys();
  std::cout << "[4] keys =";
  for (const std::string& key : keys) {
    std::cout << ' ' << key;
  }
  std::cout << '\n';

  std::cout << "[5] missing = " << registry.get("missing").value_or("none")
            << '\n';
}
