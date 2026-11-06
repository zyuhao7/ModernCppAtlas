#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

int main() {
  // Fixed seed keeps the snapshot stable; production code seeds from
  // std::random_device.
  std::mt19937 rng(2024);

  std::uniform_int_distribution<int> dice(1, 6);
  std::vector<int> rolls;
  for (int i = 0; i < 6; ++i) {
    rolls.push_back(dice(rng));
  }
  std::sort(rolls.begin(), rolls.end());
  std::cout << "dice:";
  for (int value : rolls) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';

  std::vector<int> cards{1, 2, 3, 4, 5};
  std::shuffle(cards.begin(), cards.end(), rng);
  std::cout << "cards:";
  for (int value : cards) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';
}
