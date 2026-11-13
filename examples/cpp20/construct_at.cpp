#include <cstddef>
#include <iostream>
#include <memory>
#include <new>
#include <string>

struct Point {
  int x;
  int y;
};

int main() {
  alignas(Point) std::byte storage[sizeof(Point)];
  Point* p = std::construct_at(reinterpret_cast<Point*>(storage), 3, 4);
  std::cout << "[1] point = " << p->x << ',' << p->y << '\n';
  std::destroy_at(p);

  static constexpr int folded = [] {
    int buffer[1];
    std::construct_at(buffer, 21);
    return *buffer * 2;
  }();
  std::cout << "[2] constexpr = " << folded << '\n';

  std::string buffer[2];
  std::construct_at(buffer, "hello");
  std::construct_at(buffer + 1, 3, 'z');
  std::cout << "[3] strings = " << buffer[0] << ' ' << buffer[1] << '\n';
  std::destroy_at(buffer + 1);
  std::destroy_at(buffer);

  void* heap = ::operator new(sizeof(Point));
  Point* slot = std::construct_at(static_cast<Point*>(heap), 7, 8);
  std::cout << "[4] slot = " << slot->x << ',' << slot->y << '\n';
  std::destroy_at(slot);
  ::operator delete(heap);

  int scalar = 0;
  int* sp = std::construct_at(&scalar, 42);
  std::cout << "[5] scalar = " << *sp << '\n';

  alignas(Point) std::byte raw[sizeof(Point)];
  Point* q = std::construct_at(reinterpret_cast<Point*>(raw), Point{1, 2});
  std::cout << "[6] moved = " << q->x << ',' << q->y << '\n';
  std::destroy_at(q);
}
