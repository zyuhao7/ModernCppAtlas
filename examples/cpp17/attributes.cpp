#include <iostream>
#include <type_traits>

[[nodiscard]] int compute(int value) { return value * 2; }

[[deprecated("use compute instead")]] int compute_old(int value) {
  return value;
}

struct [[nodiscard]] Outcome {
  int code;
};

Outcome make_outcome(int code) { return Outcome{code}; }

int classify(int value) {
  int result = 0;
  switch (value) {
    case 0:
      result += 1;
      [[fallthrough]];
    case 1:
      result += 10;
      break;
    default:
      break;
  }
  return result;
}

int read_level(int value, [[maybe_unused]] int mode) { return value; }

int main() {
  [[maybe_unused]] int cached = compute(3);
  std::cout << "[1] compute(3) = " << compute(3) << '\n';

  Outcome outcome = make_outcome(7);
  std::cout << "[2] outcome = " << outcome.code << '\n';

  std::cout << "[3] classify(0) = " << classify(0)
            << ", classify(1) = " << classify(1)
            << ", classify(2) = " << classify(2) << '\n';

  std::cout << "[4] read_level = " << read_level(5, 0) << '\n';

  static_assert(std::is_same<decltype(&compute), int (*)(int)>::value,
                "attributes do not change the function type");
  std::cout << "[5] attribute not in type = 1\n";
}
