#pragma once
#include "application.h"
#include "json.h"

#include <QDate>
#include <chrono>
#include <map>

class Period;
class QDate;

class Plan
{
public:
  explicit Plan(const nlohmann::json& data);
  explicit Plan();
  [[nodiscard]] nlohmann::json to_json() const noexcept;
  [[nodiscard]] std::chrono::minutes planned_working_time(const QDate& date,
                                                          std::chrono::minutes actual_working_time) const;
  [[nodiscard]] const std::chrono::minutes& overtime_offset() const noexcept;
  [[nodiscard]] const QDate& start() const noexcept;

protected:
  [[nodiscard]] virtual std::chrono::minutes planned_normal_working_time(const QDate& date) const noexcept = 0;

private:
  QDate m_start = Application::current_date_time().date();
  std::chrono::minutes m_overtime_offset{0};

  enum class Kind { Normal, Sick, Holiday, HalfHoliday, Vacation, HalfVacation, HalfVacationHalfHoliday };
  using KindMap = std::map<QDate, Kind>;
  KindMap m_kinds;
  [[nodiscard]] KindMap::const_iterator find_kind(const QDate& date) const;
};

class FullTimePlan : public Plan
{
public:
  using Plan::Plan;
  [[nodiscard]] std::chrono::minutes planned_normal_working_time(const QDate& date) const noexcept override;
};
