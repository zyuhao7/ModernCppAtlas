#include <iostream>

[[nodiscard]] int compute(int value) { return value * 2; }

[[deprecated("use compute instead")]] int compute_old(int value) { return value; }

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

int main() {
  [[maybe_unused]] int cached = compute(3);
  Outcome outcome = make_outcome(7);
  std::cout << outcome.code << ' ' << classify(0) << '\n';
}
