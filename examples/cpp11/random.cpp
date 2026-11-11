#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

int main() {
  // 固定种子让快照稳定；生产代码应从 std::random_device 播种。
  std::mt19937 rng(2024);

  // 场景 1：引擎产生原始比特流，分布负责映射到区间。
  // 同一引擎状态在不同分布间共享，因此顺序调用会影响结果。
  std::uniform_int_distribution<int> dice(1, 6);
  std::vector<int> rolls;
  for (int i = 0; i < 6; ++i) {
    rolls.push_back(dice(rng));
  }
  std::sort(rolls.begin(), rolls.end());
  std::cout << "[1] dice:";
  for (int value : rolls) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';

  // 场景 2：浮点分布，结果落在 [0, 1)。
  std::uniform_real_distribution<double> unit(0.0, 1.0);
  double sample = unit(rng);
  std::cout << "[2] in [0,1) = " << std::boolalpha
            << (sample >= 0.0 && sample < 1.0) << '\n';

  // 场景 3：伯努利分布按概率产生 bool。
  std::bernoulli_distribution coin(0.5);
  std::cout << "[3] coin = " << (coin(rng) ? "heads" : "tails") << '\n';

  // 场景 4：shuffle 用同一引擎打乱序列，结果是排列而不改变元素集合。
  std::vector<int> cards{1, 2, 3, 4, 5};
  std::shuffle(cards.begin(), cards.end(), rng);
  std::cout << "[4] cards:";
  for (int value : cards) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';

  // 场景 5：固定种子可复现。两个独立引擎用同一种子，
  // 各自第一次取值必然相同。
  std::mt19937 left(7);
  std::mt19937 right(7);
  std::cout << "[5] reproducible = " << (left() == right()) << '\n';

  // 场景 6：random_device 是非确定性熵源，只适合播种而非批量生成；
  // 它的具体取值不可复现，因此只检查播种后的结果落在范围内。
  std::random_device device;
  std::mt19937 seeded(device());
  std::uniform_int_distribution<int> bounded(1, 100);
  int picked = bounded(seeded);
  std::cout << "[6] seeded result in [1,100] = "
            << (picked >= 1 && picked <= 100) << '\n';
}
