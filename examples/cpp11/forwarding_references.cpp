#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct Counted {
  static int copies;
  static int moves;
  Counted() = default;
  Counted(const Counted&) { ++copies; }
  Counted(Counted&&) noexcept { ++moves; }
  Counted& operator=(const Counted&) {
    ++copies;
    return *this;
  }
  Counted& operator=(Counted&&) noexcept {
    ++moves;
    return *this;
  }
};
int Counted::copies = 0;
int Counted::moves = 0;

class Buffer {
 public:
  Buffer() { store_.reserve(4); }

  template <typename T>
  void add(T&& value) {
    store_.push_back(std::forward<T>(value));
  }

  const std::vector<std::string>& items() const { return store_; }

 private:
  std::vector<std::string> store_;
};

template <typename T>
const char* category(T&&) {
  return std::is_lvalue_reference<T>::value
             ? (std::is_const<typename std::remove_reference<T>::type>::value
                    ? "const lvalue"
                    : "lvalue")
             : "rvalue";
}

template <typename T>
void sink_forward(T&& value, std::vector<Counted>& out) {
  out.push_back(std::forward<T>(value));
}

template <typename T>
void sink_copy(T&& value, std::vector<Counted>& out) {
  out.push_back(value);  // 具名形参永远是左值，右值性在这里丢失
}

template <typename T>
using Ref = T&&;

int main() {
  // 场景 1：左值实参推导为 T&，按引用绑定后拷贝进容器；
  // 临时对象推导为 T，按右值引用绑定后移动进容器。
  Buffer buffer;
  std::string text = "atlas";
  buffer.add(text);
  buffer.add(std::string("moved"));
  std::cout << "[1] items =";
  for (const auto& item : buffer.items()) std::cout << ' ' << item;
  std::cout << ", source text = " << text << '\n';

  // 场景 2：同一个模板按实参值类别推导出不同的 T。
  const std::string& const_ref = text;
  std::cout << "[2] text = " << category(text)
            << ", temp = " << category(std::string("tmp"))
            << ", const_ref = " << category(const_ref) << '\n';

  // 场景 3：转发到下一层时，std::forward 保留右值性；
  // 不转发则具名形参退化为左值，右值也会走拷贝。
  std::vector<Counted> forwarded;
  std::vector<Counted> copied;
  forwarded.reserve(1);
  copied.reserve(1);
  Counted source;
  sink_forward(std::move(source), forwarded);
  sink_copy(Counted(), copied);
  std::cout << "[3] forwarded moves = " << Counted::moves
            << ", unforwarded copies = " << Counted::copies << '\n';

  // 场景 4：auto&& 也是转发引用，绑定左值时推导为左值引用，
  // 通过它修改会作用到原对象。
  auto&& bound = text;
  bound += "!";
  std::cout << "[4] text after auto&& write = " << text << '\n';

  // 场景 5：引用折叠只有 && && 折叠为 &&，其余组合都折叠为 &。
  bool lvalue_collapse = std::is_same<Ref<int&>, int&>::value;
  bool rvalue_keep = std::is_same<Ref<int>, int&&>::value;
  std::cout << "[5] & && -> & = " << std::boolalpha << lvalue_collapse
            << ", && -> && = " << rvalue_keep << '\n';
}
