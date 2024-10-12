#pragma once

#include "application.h"
#include "fmt.h"
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

  enum class Kind { Normal, Sick, Holiday, HalfHoliday, Vacation, HalfVacation, HalfVacationHalfHoliday };
  using KindMap = std::map<QDate, Kind>;
  [[nodiscard]] KindMap::const_iterator find_kind(const QDate& date) const;

protected:
  [[nodiscard]] virtual std::chrono::minutes planned_normal_working_time(const QDate& date) const noexcept = 0;

private:
  QDate m_start = Application::current_date_time().date();
  std::chrono::minutes m_overtime_offset{0};

  KindMap m_kinds;
};

class FullTimePlan : public Plan
{
public:
  using Plan::Plan;
  [[nodiscard]] std::chrono::minutes planned_normal_working_time(const QDate& date) const noexcept override;
};

template<> struct fmt::formatter<Plan::Kind> : fmt::formatter<std::string>
{
  [[nodiscard]] static auto format(Plan::Kind kind, fmt::format_context& ctx)
  {
    const auto str = [kind]() {
      switch (kind) {
        using enum Plan::Kind;
      case Normal:
        return "Normal";
      case Sick:
        return "Sick";
      case Vacation:
        return "Vacation";
      case Holiday:
        return "Holiday";
      case HalfHoliday:
        return "Half Holiday";
      case HalfVacation:
        return "Half Vacation";
      case HalfVacationHalfHoliday:
        return "Half Vacation, Half Holiday";
      }
      Q_UNREACHABLE();
    }();
    using std::chrono_literals::operator""min;
    return fmt::format_to(ctx.out(), "{}", str);
  }
};
