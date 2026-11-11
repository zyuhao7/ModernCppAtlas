#include <iostream>
#include <memory>
#include <utility>
#include <vector>

struct Connection {
  explicit Connection(int id) : id(id) {}
  ~Connection() { std::cout << "close " << id << '\n'; }
  int id;
};

std::unique_ptr<Connection> connect(int id) {
  return std::unique_ptr<Connection>(new Connection(id));
}

int main() {
  // 场景 1：std::move 转移唯一所有权。赋值先把 primary 原来的连接
  // 释放（打印 close 1），再接管 backup 的对象；backup 变为空。
  {
    std::unique_ptr<Connection> primary = connect(1);
    std::unique_ptr<Connection> backup = connect(2);
    primary = std::move(backup);
    std::cout << "[1] held = " << (primary ? "yes" : "no")
              << ", backup = " << (backup ? "yes" : "no")
              << ", id = " << primary->id << '\n';
  }

  // 场景 2：离开作用域自动释放，不需要手写 delete。
  {
    std::unique_ptr<Connection> scoped = connect(3);
    std::cout << "[2] in scope id = " << scoped->id << '\n';
  }

  // 场景 3：release 交出裸指针并清空自身；reset 重新接管所有权，
  // 之后仍由 unique_ptr 负责释放，避免手工 delete 遗漏。
  {
    std::unique_ptr<Connection> owner = connect(4);
    Connection* raw = owner.release();
    std::cout << "[3] after release owner = " << (owner ? "yes" : "no")
              << ", raw id = " << raw->id << '\n';
    owner.reset(raw);
  }

  // 场景 4：删除器是类型的一部分。这里用无捕获 lambda 定制释放动作，
  // 对象超出作用域时先打印 custom close，再释放内存。
  {
    auto deleter = [](Connection* connection) {
      std::cout << "custom close " << connection->id << '\n';
      delete connection;
    };
    std::unique_ptr<Connection, decltype(deleter)> custom(new Connection(5),
                                                          deleter);
    std::cout << "[4] custom id = " << custom->id << '\n';
  }

  // 场景 5：容器保存 unique_ptr，元素只能移动，不能复制。
  // 容器析构时按元素顺序自动释放每个对象。
  {
    std::vector<std::unique_ptr<Connection>> pool;
    pool.push_back(connect(6));
    std::cout << "[5] pool size = " << pool.size()
              << ", id = " << pool.front()->id << '\n';
  }
}
