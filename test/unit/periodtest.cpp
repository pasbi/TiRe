#include "period.h"
#include "fmt.h"
#include <gtest/gtest.h>

namespace
{
struct PeriodConstrainTestParameter
{
  Period candidate;
  Period constrained_period;
  friend void PrintTo(const PeriodConstrainTestParameter& p, std::ostream* const os)
  {
    *os << fmt::format("{} is expected to be constrained to {}", p.candidate, p.constrained_period);
  }
};

using std::chrono_literals::operator""y;
using std::chrono::August;
using std::chrono::December;
using std::chrono::January;
using std::chrono::June;
using std::chrono::last;
using std::chrono::October;
using std::chrono::September;

const QDate start_date = 2024y / October / 16;
const QDate today = 2024y / September / 23;
;

using PeriodConstrainTestFixture = ::testing::TestWithParam<PeriodConstrainTestParameter>;

}  // namespace

void PrintTo(const QDate& date, std::ostream* const os)
{
  *os << fmt::format("{}", date);
}

TEST_P(PeriodConstrainTestFixture, Constrain)
{
  const auto& [candidate, expected_constrained_period] = GetParam();

  const auto actual_constrained_period = candidate.constrained(::start_date, ::today);
  ASSERT_EQ(actual_constrained_period.begin(), expected_constrained_period.begin());
  ASSERT_EQ(actual_constrained_period.end(), expected_constrained_period.end());
}

INSTANTIATE_TEST_CASE_P(PeriodConstrainTests, PeriodConstrainTestFixture,
                        ::testing::Values(
                            PeriodConstrainTestParameter{
                                // candidate starts before `today` and ends after `start_date`
                                .candidate = Period{2024y / August / 17, Period::Type::Year},
                                .constrained_period = Period{start_date, Period::Type::Year},
                            },
                            PeriodConstrainTestParameter{
                                // candidate ends before `start_date`
                                .candidate = Period{2023y / August / 17, Period::Type::Year},
                                .constrained_period = Period{start_date, Period::Type::Year},
                            },
                            PeriodConstrainTestParameter{
                                // candidate starts after `today`
                                .candidate = Period{2025y / August / 17, Period::Type::Year},
                                .constrained_period = Period{start_date, Period::Type::Year},
                            },
                            PeriodConstrainTestParameter{
                                // candidate ends before `start_date`
                                .candidate = Period{2024y / June / 17, Period::Type::Month},
                                .constrained_period = Period{start_date, Period::Type::Month},
                            },
                            PeriodConstrainTestParameter{
                                // candidate starts after `today`
                                .candidate = Period{2024y / October / 17, Period::Type::Month},
                                .constrained_period = Period{today, Period::Type::Month},
                            }));
