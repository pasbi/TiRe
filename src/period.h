#pragma once

#include <QDate>

class Interval;

class Period
{
public:
  enum class Type { Year, Month, Week, Day };
  explicit Period(const QDate& date, Type type);
  [[nodiscard]] const QDate& begin() const noexcept;
  [[nodiscard]] const QDate& end() const noexcept;
  [[nodiscard]] Type type() const noexcept;
  [[nodiscard]] int minutes_overlap(const Interval& interval) const noexcept;

private:
  QDate m_begin;
  QDate m_end;
  Type m_type;
};
