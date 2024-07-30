#include "projectmodel.h"
#include "exceptions.h"
#include "project.h"
#include <ranges>

namespace
{

[[nodiscard]] Project& find(const std::vector<std::unique_ptr<Project>>& projects, const Project::Type type)
{
  const auto pred = [type](const auto& project) { return project->type() == type; };
  if (const auto it = std::ranges::find_if(projects, pred); it != projects.end()) {
    return **it;
  }
  throw RuntimeError("Failed to find project of type {}.", type);
}

}  // namespace

ProjectModel::ProjectModel()
  : m_empty_project(add(std::make_unique<Project>(Project::Type::Empty, QString{})))
  , m_holiday_project(add(std::make_unique<Project>(Project::Type::Holiday, QString{})))
  , m_sick_project(add(std::make_unique<Project>(Project::Type::Sick, QString{})))
{
}

ProjectModel::ProjectModel(std::vector<std::unique_ptr<Project>> projects)
  : m_projects(std::move(projects))
  , m_empty_project(find(m_projects, Project::Type::Empty))
  , m_holiday_project(find(m_projects, Project::Type::Holiday))
  , m_sick_project(find(m_projects, Project::Type::Sick))
{
}

ProjectModel::~ProjectModel() = default;

std::vector<Project*> ProjectModel::projects() const
{
  auto view = m_projects | std::views::transform(&std::unique_ptr<Project>::get);
  return std::vector(view.begin(), view.end());
}

Project& ProjectModel::add(std::unique_ptr<Project> project)
{
  return *m_projects.emplace_back(std::move(project));
}

std::unique_ptr<Project> ProjectModel::extract(const Project& project)
{
  const auto it = std::ranges::find(m_projects, &project, &std::unique_ptr<Project>::get);
  auto extracted_project = std::move(*it);
  m_projects.erase(it);
  return extracted_project;
}

const Project& ProjectModel::empty_project() const noexcept
{
  return m_empty_project;
}

const Project& ProjectModel::project(const std::size_t index) const
{
  return *m_projects.at(index);
}

std::size_t ProjectModel::index_of(const Project& project) const
{
  if (const auto it = std::ranges::find(m_projects, &project, &std::unique_ptr<Project>::get); it != m_projects.end()) {
    return std::distance(m_projects.begin(), it);
  }
  throw RuntimeError("Cannot find unexpected project.");
}
