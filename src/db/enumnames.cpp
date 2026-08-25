#include "db/enumnames.h"

#include "exceptions.h"

#include <QString>
#include <algorithm>
#include <array>
#include <utility>

namespace
{

// Keeping the tables next to the switches means from_db_name and db_name cannot drift apart.
constexpr auto period_type_names = std::array{
    std::pair{Period::Type::Year, "YEAR"},     std::pair{Period::Type::Month, "MONTH"},
    std::pair{Period::Type::Week, "WEEK"},     std::pair{Period::Type::Day, "DAY"},
    std::pair{Period::Type::Custom, "CUSTOM"},
};

constexpr auto plan_kind_names = std::array{
    std::pair{Plan::Kind::Normal, "NORMAL"},
    std::pair{Plan::Kind::Sick, "SICK"},
    std::pair{Plan::Kind::Holiday, "HOLIDAY"},
    std::pair{Plan::Kind::HalfHoliday, "HALF_HOLIDAY"},
    std::pair{Plan::Kind::Vacation, "VACATION"},
    std::pair{Plan::Kind::HalfVacation, "HALF_VACATION"},
    std::pair{Plan::Kind::HalfVacationHalfHoliday, "HALF_VACATION_HALF_HOLIDAY"},
};

template<typename Enum, std::size_t n> [[nodiscard]] Enum
from_name(const std::array<std::pair<Enum, const char*>, n>& names, const QString& name, const char* const type_name)
{
  const auto it = std::ranges::find(names, name, [](const auto& pair) { return QString::fromLatin1(pair.second); });
  if (it == names.end()) {
    throw DatabaseError("'{}' is not a valid {}.", name.toStdString(), type_name);
  }
  return it->first;
}

}  // namespace

QString db_name(const Period::Type type)
{
  switch (type) {
  case Period::Type::Year:
    return QStringLiteral("YEAR");
  case Period::Type::Month:
    return QStringLiteral("MONTH");
  case Period::Type::Week:
    return QStringLiteral("WEEK");
  case Period::Type::Day:
    return QStringLiteral("DAY");
  case Period::Type::Custom:
    return QStringLiteral("CUSTOM");
  }
  throw DatabaseError("Unknown Period::Type: {}.", static_cast<int>(type));
}

QString db_name(const Plan::Kind kind)
{
  switch (kind) {
  case Plan::Kind::Normal:
    return QStringLiteral("NORMAL");
  case Plan::Kind::Sick:
    return QStringLiteral("SICK");
  case Plan::Kind::Holiday:
    return QStringLiteral("HOLIDAY");
  case Plan::Kind::HalfHoliday:
    return QStringLiteral("HALF_HOLIDAY");
  case Plan::Kind::Vacation:
    return QStringLiteral("VACATION");
  case Plan::Kind::HalfVacation:
    return QStringLiteral("HALF_VACATION");
  case Plan::Kind::HalfVacationHalfHoliday:
    return QStringLiteral("HALF_VACATION_HALF_HOLIDAY");
  }
  throw DatabaseError("Unknown Plan::Kind: {}.", static_cast<int>(kind));
}

Period::Type period_type_from_db_name(const QString& name)
{
  return ::from_name(period_type_names, name, "Period::Type");
}

Plan::Kind plan_kind_from_db_name(const QString& name)
{
  return ::from_name(plan_kind_names, name, "Plan::Kind");
}
