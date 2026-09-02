#include "db/enumnames.h"

#include "exceptions.h"
#include "fmt.h"

#include <QString>
#include <array>
#include <gtest/gtest.h>

TEST(EnumNamesTest, PeriodTypeRoundTrip)
{
  using enum Period::Type;
  for (const auto type : {Year, Month, Week, Day, Custom}) {
    EXPECT_EQ(type, period_type_from_db_name(db_name(type)));
  }
}

TEST(EnumNamesTest, PlanKindRoundTrip)
{
  using enum Plan::Kind;
  for (const auto kind : {Normal, Sick, Holiday, HalfHoliday, Vacation, HalfVacation, HalfVacationHalfHoliday}) {
    EXPECT_EQ(kind, plan_kind_from_db_name(db_name(kind)));
  }
}

TEST(EnumNamesTest, NamesArePinned)
{
  // These literals are the on-disk format. Changing one silently orphans existing rows, so pin
  // them here: a rename must be a deliberate migration, not an accident.
  EXPECT_EQ(QStringLiteral("CUSTOM"), db_name(Period::Type::Custom));
  EXPECT_EQ(QStringLiteral("YEAR"), db_name(Period::Type::Year));
  EXPECT_EQ(QStringLiteral("NORMAL"), db_name(Plan::Kind::Normal));
  EXPECT_EQ(QStringLiteral("HALF_VACATION_HALF_HOLIDAY"), db_name(Plan::Kind::HalfVacationHalfHoliday));
}

TEST(EnumNamesTest, PersistenceNamesAreNotDisplayNames)
{
  // The fmt formatters go through QObject::tr(), so they are locale-dependent and unusable as a
  // storage format. This guards against anyone re-coupling the two.
  EXPECT_NE(QString::fromStdString(fmt::format("{}", Plan::Kind::HalfHoliday)), db_name(Plan::Kind::HalfHoliday));
}

TEST(EnumNamesTest, UnknownNameThrows)
{
  EXPECT_THROW(period_type_from_db_name(QStringLiteral("nonsense")), DatabaseError);
  EXPECT_THROW(plan_kind_from_db_name(QStringLiteral("nonsense")), DatabaseError);
  // Case matters: the stored form is upper case.
  EXPECT_THROW(period_type_from_db_name(QStringLiteral("year")), DatabaseError);
}
