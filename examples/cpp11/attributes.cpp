#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

// 场景 1：[[noreturn]] 表示函数不会正常返回，但可以抛异常。
[[noreturn]] void fail(const char* message) {
  throw std::runtime_error(message);
}

// 场景 2：非 void 函数用不返回的 default 分支收尾，不需要假 return。
int classify(int code) {
  switch (code) {
    case 1:
      return 100;
    case 2:
      return 200;
    default:
      fail("unexpected code");
  }
}

// 场景 3：属性放在模板声明与返回类型之间，属于标准位置。
template <typename Message>
[[noreturn]] void relay(Message&& message) {
  throw std::runtime_error(message);
}

// 场景 5：属性写在首次声明上，定义可以省略，契约仍然生效。
[[noreturn]] void bail(int code);

void bail(int code) {
  throw std::runtime_error("exit " + std::to_string(code));
}

int main() {
  try {
    fail("boom");
  } catch (const std::exception& error) {
    std::cout << "[1] caught: " << error.what() << '\n';
  }

  std::cout << "[2] classify(1) = " << classify(1)
            << ", classify(2) = " << classify(2) << '\n';
  try {
    classify(9);
  } catch (const std::exception& error) {
    std::cout << "[2] classify(9) throws: " << error.what() << '\n';
  }

  try {
    relay(std::string("relayed"));
  } catch (const std::exception& error) {
    std::cout << "[3] template caught: " << error.what() << '\n';
  }

  // 场景 4：属性不进入函数类型，也不改变 ABI。
  void (*pointer)(const char*) = &fail;
  std::cout << "[4] attribute in function type = "
            << std::is_same<decltype(&fail), void (*)(const char*)>::value
            << '\n';
  try {
    pointer("through pointer");
  } catch (const std::exception& error) {
    std::cout << "[4] pointer call caught: " << error.what() << '\n';
  }

  try {
    bail(7);
  } catch (const std::exception& error) {
    std::cout << "[5] declaration-only attribute caught: " << error.what()
              << '\n';
  }
}
