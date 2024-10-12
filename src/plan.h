#pragma once

#include "application.h"
#include "fmt.h"
#include "json.h"
#include "period.h"

#include <QAbstractTableModel>
#include <QDate>
#include <chrono>

class QDate;

class Plan : QAbstractTableModel
{
public:
  static constexpr auto date_column = 0;
  static constexpr auto kind_column = 1;
  explicit Plan(const nlohmann::json& data);
  explicit Plan();
  [[nodiscard]] nlohmann::json to_json() const noexcept;
  [[nodiscard]] std::chrono::minutes planned_working_time(const QDate& date,
                                                          std::chrono::minutes actual_working_time) const;
  [[nodiscard]] const std::chrono::minutes& overtime_offset() const noexcept;
  [[nodiscard]] const QDate& start() const noexcept;

  enum class Kind { Normal, Sick, Holiday, HalfHoliday, Vacation, HalfVacation, HalfVacationHalfHoliday };
  [[nodiscard]] Kind find_kind(const QDate& date) const;

  [[nodiscard]] int columnCount(const QModelIndex& parent) const override;
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;

  struct Entry
  {
    Period period;
    Kind kind;
  };

protected:
  [[nodiscard]] virtual std::chrono::minutes planned_normal_working_time(const QDate& date) const noexcept = 0;

private:
  QDate m_start = Application::current_date_time().date();
  std::chrono::minutes m_overtime_offset{0};

  std::vector<Entry> m_periods;
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
    return format_to(ctx.out(), "{}", str);
  }
};

void to_json(nlohmann::json& j, const Plan::Entry& value);
void from_json(const nlohmann::json& j, Plan::Entry& value);
void to_json(nlohmann::json& j, const Plan::Kind& value);
void from_json(const nlohmann::json& j, Plan::Kind& value);
