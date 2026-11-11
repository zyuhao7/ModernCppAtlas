#include <iostream>
#include <string>
#include <type_traits>

// 场景 1：两个重载用互斥的 enable_if 条件限制在整型和浮点类型上。
// 不满足条件的重载从候选集中消失，调用点在编译期完成选择。
template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type twice(T value) {
  return value + value;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type twice(
    T value) {
  return value * 2;
}

// 场景 2：自定义特征继承 true_type/false_type，用偏特化识别 const T*。
template <typename T>
struct is_pointer_to_const : std::false_type {};

template <typename T>
struct is_pointer_to_const<const T*> : std::true_type {};

// 场景 3：类型变换只改类型，用 ::type 取结果。
template <typename T>
using raw_value = typename std::decay<T>::type;

// 场景 4：conditional 在两个类型之间选择，条件必须是编译期常量。
using word_t =
    typename std::conditional<(sizeof(void*) == 8), long long, int>::type;

int main() {
  // 场景 1：enable_if 让两个同名函数只对各自类型可见。
  std::cout << "[1] twice(21) = " << twice(21)
            << ", twice(1.5) = " << twice(1.5) << '\n';

  // 场景 2：内置特征与自定义偏特化特征都通过 ::value 回答布尔问题。
  std::cout << "[2] is_integral<int> = " << std::boolalpha
            << std::is_integral<int>::value
            << ", is_pointer_to_const<const int*> = "
            << is_pointer_to_const<const int*>::value
            << ", is_pointer_to_const<int*> = "
            << is_pointer_to_const<int*>::value << '\n';

  // 场景 3：decay 同时去掉引用、顶层 const，并把数组退化为指针。
  static_assert(std::is_same<raw_value<const int&>, int>::value,
                "decay removes reference and const");
  static_assert(std::is_same<raw_value<int[3]>, int*>::value,
                "decay turns array into pointer");
  std::cout << "[3] decay<const int&> is int = "
            << std::is_same<raw_value<const int&>, int>::value
            << ", decay<int[3]> is int* = "
            << std::is_same<raw_value<int[3]>, int*>::value << '\n';

  // 场景 4：remove_reference 只去引用，保留 const，与 decay 形成对照。
  std::cout << "[4] remove_reference<const int&> is const int = "
            << std::is_same<typename std::remove_reference<const int&>::type,
                            const int>::value
            << '\n';

  // 场景 5：conditional 按平台选择整数宽度，is_convertible 回答隐式转换问题。
  std::cout << "[5] word_t is 64-bit = "
            << std::is_same<word_t, long long>::value
            << ", int convertible to double = "
            << std::is_convertible<int, double>::value
            << ", string convertible to int = "
            << std::is_convertible<std::string, int>::value << '\n';
}
