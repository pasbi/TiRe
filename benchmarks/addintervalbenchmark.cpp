#include "createlargeproject.h"
#include "intervalmodel.h"
#include "plan.h"
#include "projectmodel.h"
#include "timesheet.h"

class RAIITimer
{
public:
  explicit RAIITimer(std::string name) : m_name(std::move(name)), m_start(clock::now())
  {
  }

  ~RAIITimer()
  {
    const auto end = clock::now();
    using std::chrono_literals::operator""ms;
    fmt::print("{}: {:< 8.4}ms\n", m_name, (end - m_start) / 1.0ms);
  }

  RAIITimer(const RAIITimer& other) = delete;
  RAIITimer(RAIITimer&& other) = delete;
  RAIITimer& operator=(const RAIITimer& other) = delete;
  RAIITimer& operator=(RAIITimer&& other) = delete;

  using clock = std::chrono::steady_clock;

private:
  const std::string m_name;
  const std::chrono::time_point<clock> m_start;
};

int main()
{
  for (const int exponent : std::views::iota(0, 20)) {
    const auto n = std::pow(2, exponent);
    RAIITimer timer(fmt::format("n = {:8}", n));
    const auto time_sheet = ::create_large_time_sheet(n);
  }
}
