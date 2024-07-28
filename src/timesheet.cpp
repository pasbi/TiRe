#include "timesheet.h"

#include "intervalmodel.h"
#include "projectmodel.h"

TimeSheet::TimeSheet()
  : m_project_model(std::make_unique<ProjectModel>()), m_interval_model(std::make_unique<IntervalModel>())
{
}

TimeSheet::TimeSheet(std::unique_ptr<ProjectModel> project_model, std::unique_ptr<IntervalModel> interval_model)
  : m_project_model(std::move(project_model)), m_interval_model(std::move(interval_model))
{
}

IntervalModel& TimeSheet::interval_model() const noexcept
{
  return *m_interval_model;
}

ProjectModel& TimeSheet::project_model() const noexcept
{
  return *m_project_model;
}