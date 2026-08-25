#pragma once

#include <memory>

class AbstractTimeSheetRepository;
class Plan;
class Period;
class QDate;
class IntervalModel;
class ProjectModel;

class TimeSheet
{
public:
  /** @brief Creates an empty timesheet that does not persist anything. */
  explicit TimeSheet();
  /** @brief Creates an empty timesheet whose models write through to @p repository. */
  explicit TimeSheet(AbstractTimeSheetRepository& repository);
  explicit TimeSheet(std::unique_ptr<ProjectModel> project_model, std::unique_ptr<IntervalModel> interval_model,
                     std::unique_ptr<Plan> plan);
  // Declared here and defined in the .cpp so the models can stay forward-declared.
  ~TimeSheet();
  TimeSheet(const TimeSheet&) = delete;
  TimeSheet& operator=(const TimeSheet&) = delete;
  TimeSheet(TimeSheet&&) noexcept;
  TimeSheet& operator=(TimeSheet&&) noexcept;
  [[nodiscard]] IntervalModel& interval_model() const noexcept;
  [[nodiscard]] ProjectModel& project_model() const noexcept;
  [[nodiscard]] Plan& plan() const noexcept;

private:
  std::unique_ptr<ProjectModel> m_project_model;
  std::unique_ptr<IntervalModel> m_interval_model;
  std::unique_ptr<Plan> m_plan;
};
