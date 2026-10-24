#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

struct Account {
  explicit Account(int initial) : balance(initial) {}
  int balance;
  std::mutex mu;
};

void transfer(Account &from, Account &to, int amount) {
  std::scoped_lock lock(from.mu, to.mu);
  if (from.balance < amount) return;
  from.balance -= amount;
  to.balance += amount;
}

int snapshot(Account &account) {
  std::scoped_lock lock(account.mu);
  return account.balance;
}

int main() {
  Account a(1000);
  Account b(1000);

  std::vector<std::thread> workers;
  for (int i = 0; i < 50; ++i) {
    workers.emplace_back([&] { transfer(a, b, 7); });
    workers.emplace_back([&] { transfer(b, a, 3); });
  }
  for (std::thread &worker : workers) worker.join();

  const int a_balance = snapshot(a);
  const int b_balance = snapshot(b);
  std::cout << "total " << a_balance + b_balance << '\n';
  std::cout << "a " << a_balance << " b " << b_balance << '\n';

  transfer(b, a, 5000);
  std::cout << "unchanged " << snapshot(a) << ' ' << snapshot(b) << '\n';
}
