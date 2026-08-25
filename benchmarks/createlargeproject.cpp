#include "intervalmodel.h"
#include "plan.h"
#include "projectmodel.h"
#include "timesheet.h"

#include <random>

template<typename RNG> class TimeAdvancer
{
public:
  explicit TimeAdvancer(RNG& rng) : m_rng(rng)
  {
  }

  QDateTime next()
  {
    using std::chrono_literals::operator""min;
    using std::chrono_literals::operator""h;
    static constexpr auto min_advance = 1min;
    static constexpr auto max_advance = 24h;
    static std::uniform_int_distribution minute_distribution(min_advance / 1min, max_advance / 1min);

    using std::chrono_literals::operator""s;
    const auto ret = current_timestamp;
    current_timestamp = current_timestamp.addSecs(minute_distribution(m_rng) * 60);
    return ret;
  }

private:
  RNG& m_rng;

  // current_timestamp needs to be initialized with an arbitrary date-time.
  // It's important that it doesn't change to ensure reproducible results.
  QDateTime current_timestamp = {QDate{2025, 1, 16}, QTime{21, 02, 45}};
};

TimeSheet create_large_time_sheet(const int interval_count)
{
  TimeSheet ts;
  const auto projects = std::vector{
      &ts.project_model().add(std::make_unique<Project>("P1", Qt::red)),
      &ts.project_model().add(std::make_unique<Project>("P2", Qt::green)),
      &ts.project_model().add(std::make_unique<Project>("P3", Qt::blue)),
      &ts.project_model().add(std::make_unique<Project>("P4", Qt::yellow)),
      &ts.project_model().add(std::make_unique<Project>("P5", Qt::magenta)),
      &ts.project_model().add(std::make_unique<Project>("P6", Qt::cyan)),
  };

  // Use a constant seed to ensure reproducible results.
  static constexpr auto seed = 42;
  std::mt19937 rng{seed};
  std::uniform_int_distribution project_distribution(0, static_cast<int>(projects.size()) - 1);

  auto time_advancer = TimeAdvancer{rng};

  for (int i = 0; i < interval_count; ++i) {
    const auto* const project = projects.at(project_distribution(rng));
    auto interval = std::make_unique<Interval>(project);
    interval->swap_begin(time_advancer.next());
    interval->swap_begin(time_advancer.next());
    ts.interval_model().add(std::move(interval));
  }
  return ts;
}
