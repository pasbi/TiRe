#include "timesheet.h"

#include "intervalmodel.h"
#include "period.h"
#include "plan.h"
#include "projectmodel.h"

TimeSheet::TimeSheet()
  : m_project_model(std::make_unique<ProjectModel>())
  , m_interval_model(std::make_unique<IntervalModel>())
  , m_plan(std::make_unique<FullTimePlan>())
{
}

TimeSheet::TimeSheet(std::unique_ptr<ProjectModel> project_model, std::unique_ptr<IntervalModel> interval_model,
                     std::unique_ptr<Plan> plan)
  : m_project_model(std::move(project_model)), m_interval_model(std::move(interval_model)), m_plan(std::move(plan))
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

Plan& TimeSheet::plan() const noexcept
{
  return *m_plan;
}
