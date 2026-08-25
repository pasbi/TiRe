#pragma once

#include "db/entityid.h"

#include <QDateTime>
#include <compare>

class Project;
class Period;

/**
 * @class Interval interval.h "interval.h"
 * @brief A Period represents a named time range with well-defined begin and optional end.
 * At first glance, it resembles the Period, however, its purpose is very different.
 * - In contrast to a Period, an Interval is a stored entity with its own identity (see EntityId);
 *   a Period is a plain value.
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

  explicit Interval(const Project* project);

  /**
   * @name Mutators
   * Each swaps in a new value and returns the previous one, which is what makes them usable as
   * their own inverse in a ModifyCommand.
   *
   * @warning Mutating an Interval that an IntervalModel already owns must go through
   * make_modify_interval_command(), or the change will not reach the database. These setters are
   * deliberately unaware of persistence, because an Interval is routinely built and filled in
   * before it is added to a model, when there is no row to update yet.
   * @{
   */
  const Project* swap_project(const Project* project) noexcept;
  QDateTime swap_begin(QDateTime begin);
  QDateTime swap_end(QDateTime end);
  /** @} */

  [[nodiscard]] const Project* project() const noexcept;
  [[nodiscard]] const QDateTime& begin() const noexcept;
  [[nodiscard]] const QDateTime& end() const noexcept;
  [[nodiscard]] QString duration_text() const;
  [[nodiscard]] std::chrono::minutes duration() const;
  [[nodiscard]] Period period() const;

  /** @brief Identity of this interval's row, invalid until it has been persisted. */
  [[nodiscard]] EntityId id() const noexcept;
  void set_id(EntityId id) noexcept;

private:
  const Project* m_project;
  QDateTime m_begin;
  QDateTime m_end;
  EntityId m_id;
};
