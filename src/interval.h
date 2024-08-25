#pragma once

#include <QDateTime>
#include <compare>
#include <nlohmann/adl_serializer.hpp>

class Project;
class Period;

/**
 * @class Interval interval.h "interval.h"
 * @brief A Period represents a named time range with well-defined begin and optional end.
 * At first glance, it resembles the Period, however, its purpose is very different.
 * - In contrast to a Period, an Interval can be serialized.
 * - The Interval usually reflects a particular time range defined only within
 *   this timesheet (e.g., Friday, November 1st from 8:15 to 10:23).
 * - The Interval has a precision of a minute, while Period is precise only up to a day.
 * - The Interval is bound to a project, the Period is free.
 * - The Interval may not have an end, it is considered to be ongoing if it hasn't.
 * @see Period
 */
class Interval
{
public:
  friend std::weak_ordering operator<=>(const Interval& a, const Interval& b) noexcept;

  explicit Interval(const Project& project);
  const Project* swap_project(const Project* project) noexcept;
  [[nodiscard]] const Project& project() const noexcept;
  QDateTime swap_begin(QDateTime begin);
  QDateTime swap_end(QDateTime end);
  [[nodiscard]] const QDateTime& begin() const noexcept;
  [[nodiscard]] const QDateTime& end() const noexcept;
  [[nodiscard]] QString duration_text() const;
  [[nodiscard]] std::chrono::minutes duration() const;
  [[nodiscard]] Period period() const;

private:
  const Project* m_project;
  QDateTime m_begin;
  QDateTime m_end;
};
