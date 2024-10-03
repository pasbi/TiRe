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
  explicit Plan() = default;
  [[nodiscard]] nlohmann::json to_json() const noexcept;
  [[nodiscard]] std::chrono::minutes planned_working_time(const QDate& date) const noexcept;
  [[nodiscard]] std::chrono::minutes planned_working_time(const Period& period) const noexcept;
  [[nodiscard]] const std::chrono::minutes& overtime_offset() const noexcept;
  [[nodiscard]] const QDate start() const noexcept;

private:
  QDate m_start = Application::current_date_time().date();
  std::chrono::minutes m_overtime_offset{0};
  std::map<QDate, std::chrono::minutes> m_planned_working_time = {};
};
