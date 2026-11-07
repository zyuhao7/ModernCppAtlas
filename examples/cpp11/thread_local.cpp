#include <iostream>
#include <string>
#include <thread>

thread_local std::string trace;

void note(const std::string& value) {
  trace += value;
  std::cout << trace << '\n';
}

int main() {
  std::thread worker([] {
    note("w");
    note("x");
  });
  worker.join();
  note("m");
}
