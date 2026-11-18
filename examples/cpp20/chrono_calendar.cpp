#include <chrono>
#include <iomanip>
#include <iostream>

using namespace std::chrono;

int main() {
  // [1] construct a calendar date
  const year_month_day date{year{2024}, month{3}, day{14}};
  std::cout << "[1] " << int(date.year()) << '-' << std::setw(2)
            << std::setfill('0') << unsigned(date.month()) << '-'
            << std::setw(2) << unsigned(date.day()) << '\n';

  // [2] weekday derived from sys_days (Sunday = 0)
  const sys_days days{date};
  const weekday wd{days};
  std::cout << "[2] weekday = " << wd.c_encoding() << '\n';

  // [3] round-trip through days since epoch
  std::cout << "[3] days since epoch = " << days.time_since_epoch().count()
            << '\n';

  // [4] last day of February in a leap year
  const year_month_day_last febLast{year{2024}, month_day_last{month{2}}};
  std::cout << "[4] last day of Feb 2024 = " << unsigned(febLast.day()) << '\n';

  // [5] validation rejects an impossible date
  const year_month_day invalid{year{2023}, month{2}, day{29}};
  std::cout << "[5] 2023-02-29 ok = " << std::boolalpha << invalid.ok() << '\n';

  // [6] leap year query
  std::cout << "[6] 2024 leap = " << year{2024}.is_leap()
            << ", 2023 leap = " << year{2023}.is_leap() << '\n';
}
