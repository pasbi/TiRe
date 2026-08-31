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

const QDate start_date{2024, 10, 16};
const QDate today{2024, 9, 23};

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
  ASSERT_EQ(actual_constrained_period.dates(), expected_constrained_period.dates());
}

INSTANTIATE_TEST_CASE_P(PeriodConstrainTests, PeriodConstrainTestFixture,
                        ::testing::Values(
                            PeriodConstrainTestParameter{
                                // candidate starts before `today` and ends after `start_date`
                                .candidate = Period{QDate{2024, 8, 17}, Period::Type::Year},
                                .constrained_period = Period{start_date, Period::Type::Year},
                            },
                            PeriodConstrainTestParameter{
                                // candidate ends before `start_date`
                                .candidate = Period{QDate{2023, 8, 17}, Period::Type::Year},
                                .constrained_period = Period{start_date, Period::Type::Year},
                            },
                            PeriodConstrainTestParameter{
                                // candidate starts after `today`
                                .candidate = Period{QDate{2025, 8, 17}, Period::Type::Year},
                                .constrained_period = Period{start_date, Period::Type::Year},
                            },
                            PeriodConstrainTestParameter{
                                // candidate ends before `start_date`
                                .candidate = Period{QDate{2024, 6, 17}, Period::Type::Month},
                                .constrained_period = Period{start_date, Period::Type::Month},
                            },
                            PeriodConstrainTestParameter{
                                // candidate starts after `today`
                                .candidate = Period{QDate{2024, 10, 17}, Period::Type::Month},
                                .constrained_period = Period{today, Period::Type::Month},
                            }));
