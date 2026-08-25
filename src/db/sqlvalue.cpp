#include "db/sqlvalue.h"

#include "exceptions.h"

#include <QColor>
#include <QDateTime>
#include <QVariant>

namespace
{

// Fixed-width, second-precision, no timezone suffix. The domain works exclusively in local time
// (QDateTime::currentDateTime(), QDate::startOfDay()), so storing bare local time avoids
// conversion surprises, and the fixed width keeps string comparison equivalent to date
// comparison.
constexpr auto date_time_format = "yyyy-MM-ddTHH:mm:ss";
constexpr auto date_format = "yyyy-MM-dd";

}  // namespace

QVariant to_sql(const QDateTime& date_time)
{
  if (!date_time.isValid()) {
    return {};
  }
  return date_time.toString(date_time_format);
}

QVariant to_sql(const QDate& date)
{
  if (!date.isValid()) {
    return {};
  }
  return date.toString(date_format);
}

QVariant to_sql(const QColor& color)
{
  if (!color.isValid()) {
    return {};
  }
  return color.name(QColor::HexArgb);
}

QVariant to_sql(const std::chrono::minutes minutes)
{
  return QVariant::fromValue(static_cast<qlonglong>(minutes.count()));
}

QDateTime date_time_from_sql(const QVariant& value)
{
  if (value.isNull()) {
    return {};
  }
  auto date_time = QDateTime::fromString(value.toString(), date_time_format);
  if (!date_time.isValid()) {
    throw DatabaseError("Cannot read '{}' as a date and time.", value.toString().toStdString());
  }
  return date_time;
}

QDate date_from_sql(const QVariant& value)
{
  if (value.isNull()) {
    return {};
  }
  auto date = QDate::fromString(value.toString(), date_format);
  if (!date.isValid()) {
    throw DatabaseError("Cannot read '{}' as a date.", value.toString().toStdString());
  }
  return date;
}

QColor color_from_sql(const QVariant& value)
{
  if (value.isNull()) {
    return {};
  }
  // QColor::fromString accepts both '#rrggbb' and '#aarrggbb'.
  auto color = QColor::fromString(value.toString());
  if (!color.isValid()) {
    throw DatabaseError("Cannot read '{}' as a color.", value.toString().toStdString());
  }
  return color;
}

std::chrono::minutes minutes_from_sql(const QVariant& value)
{
  auto ok = false;
  const auto count = value.toLongLong(&ok);
  if (!ok) {
    throw DatabaseError("Cannot read '{}' as a number of minutes.", value.toString().toStdString());
  }
  return std::chrono::minutes{count};
}
