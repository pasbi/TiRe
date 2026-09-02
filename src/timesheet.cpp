#include "timesheet.h"

#include "db/abstracttimesheetrepository.h"
#include "intervalmodel.h"
#include "period.h"
#include "plan.h"
#include "projectmodel.h"

TimeSheet::TimeSheet() : TimeSheet(null_repository())
{
}

TimeSheet::TimeSheet(AbstractTimeSheetRepository& repository)
  : m_project_model(std::make_unique<ProjectModel>(repository))
  , m_interval_model(std::make_unique<IntervalModel>(repository))
  , m_plan(std::make_unique<FullTimePlan>(repository))
{
}

TimeSheet::TimeSheet(std::unique_ptr<ProjectModel> project_model, std::unique_ptr<IntervalModel> interval_model,
                     std::unique_ptr<Plan> plan)
  : m_project_model(std::move(project_model)), m_interval_model(std::move(interval_model)), m_plan(std::move(plan))
{
}

TimeSheet::~TimeSheet() = default;
TimeSheet::TimeSheet(TimeSheet&&) noexcept = default;
TimeSheet& TimeSheet::operator=(TimeSheet&&) noexcept = default;

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
