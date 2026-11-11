#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

bool is_even(int value) { return value % 2 == 0; }

bool is_large(int value) { return value > 5; }

bool is_negative(int value) { return value < 0; }

}  // namespace

int main() {
  std::vector<int> values{2, 4, 6, 9};
  std::cout << std::boolalpha;

  // 场景 1：三个量化算法分别回答 "全部" "至少一个" "一个都没有"。
  // 9 破坏全偶，6 与 9 满足大于 5，区间内没有负数。
  bool all_even = std::all_of(values.begin(), values.end(), is_even);
  bool any_large = std::any_of(values.begin(), values.end(), is_large);
  bool none_negative = std::none_of(values.begin(), values.end(), is_negative);
  std::cout << "[1] all_even = " << all_even << ", any_large = " << any_large
            << ", none_negative = " << none_negative << '\n';

  // 场景 2：空区间上全称命题为真、存在命题为假。
  // 用 all_of 校验配置前先判断 empty()，否则空配置会直接通过。
  std::vector<int> empty;
  std::cout << "[2] all_of(empty) = "
            << std::all_of(empty.begin(), empty.end(), is_even)
            << ", any_of(empty) = "
            << std::any_of(empty.begin(), empty.end(), is_even)
            << ", none_of(empty) = "
            << std::none_of(empty.begin(), empty.end(), is_even) << '\n';

  // 场景 3：copy_if 过滤并复制，配合 back_inserter 追加到目标容器，
  // 不需要预先计算有多少个元素满足条件。
  std::vector<int> evens;
  std::copy_if(values.begin(), values.end(), std::back_inserter(evens),
               is_even);
  std::cout << "[3] evens =";
  for (int value : evens) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';

  // 场景 4：需要失败位置时用 find_if_not，它返回第一个不满足谓词的
  // 迭代器；这里配合 distance 得到下标，而不是只拿到一个 bool。
  auto first_odd = std::find_if_not(values.begin(), values.end(), is_even);
  if (first_odd != values.end()) {
    std::cout << "[4] first odd = " << *first_odd
              << " at index = " << std::distance(values.begin(), first_odd)
              << '\n';
  }

  // 场景 5：谓词可以是任意可调用对象，元素也不必是整数。
  // 两个 lambda 分别检查字符串前缀与子串，表达意图比手写循环直接。
  std::vector<std::string> names{"libcurl", "libuv", "libssh2"};
  bool all_lib = std::all_of(
      names.begin(), names.end(),
      [](const std::string& name) { return name.compare(0, 3, "lib") == 0; });
  bool has_http =
      std::any_of(names.begin(), names.end(), [](const std::string& name) {
        return name.find("http") != std::string::npos;
      });
  std::cout << "[5] all_lib = " << all_lib << ", has_http = " << has_http
            << '\n';
}
