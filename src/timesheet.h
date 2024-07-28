#pragma once

#include <memory>

class IntervalModel;
class ProjectModel;

class TimeSheet
{
public:
  explicit TimeSheet();
  explicit TimeSheet(std::unique_ptr<ProjectModel> project_model, std::unique_ptr<IntervalModel> interval_model);
  [[nodiscard]] IntervalModel& interval_model() const noexcept;
  [[nodiscard]] ProjectModel& project_model() const noexcept;

private:
  std::unique_ptr<ProjectModel> m_project_model;
  std::unique_ptr<IntervalModel> m_interval_model;
};
