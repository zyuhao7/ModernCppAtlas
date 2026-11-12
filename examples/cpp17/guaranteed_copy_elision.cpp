#include <iostream>
#include <type_traits>

struct Probe {
  static int constructed;
  static int destroyed;

  Probe() { ++constructed; }
  Probe(const Probe&) = delete;
  Probe(Probe&&) = delete;
  ~Probe() { ++destroyed; }

  int value = 7;
};

int Probe::constructed = 0;
int Probe::destroyed = 0;

void reset_counters() {
  Probe::constructed = 0;
  Probe::destroyed = 0;
}

Probe make() { return Probe{}; }

Probe relay() { return make(); }

struct Holder {
  Probe item = make();
};

void consume(Probe probe) { (void)probe; }

int main() {
  {
    // prvalue 直接初始化目标对象，拷贝与移动构造都不参与。
    reset_counters();
    Probe item = make();
    std::cout << "[1] direct init: constructed = " << Probe::constructed
              << ", destroyed = " << Probe::destroyed
              << ", value = " << item.value << '\n';
  }

  {
    // 成员默认初始化器同样在目标存储上直接构造。
    reset_counters();
    Holder holder;
    std::cout << "[2] member init: constructed = " << Probe::constructed
              << ", destroyed = " << Probe::destroyed << '\n';
  }

  {
    // 形参由实参 prvalue 初始化，函数返回前只析构这一个对象。
    reset_counters();
    consume(make());
    std::cout << "[3] argument init: constructed = " << Probe::constructed
              << ", destroyed = " << Probe::destroyed << '\n';
  }

  {
    // 连续返回 prvalue 不会逐层构造临时对象。
    reset_counters();
    Probe relayed = relay();
    std::cout << "[4] relayed prvalue: constructed = " << Probe::constructed
              << ", value = " << relayed.value << '\n';
  }

  // 复制消除不改变类型属性：Probe 仍然是不可拷贝、不可移动的类型。
  std::cout
      << "[5] copyable = "
      << std::is_copy_constructible_v<Probe> << ", movable = "
      << std::is_move_constructible_v<Probe> << ", trivially copyable = "
      << std::is_trivially_copyable_v<Probe> << ", default constructible = "
      << std::is_default_constructible_v<Probe> << '\n';
}
