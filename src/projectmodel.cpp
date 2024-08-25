#include "projectmodel.h"
#include "exceptions.h"
#include "project.h"
#include <random>
#include <ranges>
#include <set>
#include <spdlog/spdlog.h>

namespace
{

using ColorSet = std::set<QColor, decltype([](const QColor& a, const QColor& b) { return a.name() < b.name(); })>;

[[nodiscard]] Project& find(const std::vector<std::unique_ptr<Project>>& projects, const Project::Type type)
{
  const auto pred = [type](const auto& project) { return project->type() == type; };
  if (const auto it = std::ranges::find_if(projects, pred); it != projects.end()) {
    return **it;
  }
  throw RuntimeError("Failed to find project of type {}.", type);
}

constexpr auto colors = std::array{
    "#01FFFE", "#FFA6FE", "#FFDB66", "#006401", "#010067", "#95003A", "#007DB5", "#FF00F6", "#FFEEE8", "#774D00",
    "#90FB92", "#0076FF", "#D5FF00", "#FF937E", "#6A826C", "#FF029D", "#FE8900", "#7A4782", "#7E2DD2", "#85A900",
    "#FF0056", "#A42400", "#00AE7E", "#683D3B", "#BDC6FF", "#263400", "#BDD393", "#00B917", "#9E008E", "#001544",
    "#C28C9F", "#FF74A3", "#01D0FF", "#004754", "#E56FFE", "#788231", "#0E4CA1", "#91D0CB", "#BE9970", "#968AE8",
    "#BB8800", "#43002C", "#DEFF74", "#00FFC6", "#FFE502", "#620E00", "#008F9C", "#98FF52", "#7544B1", "#B500FF",
    "#00FF78", "#FF6E41", "#005F39", "#6B6882", "#5FAD4E", "#A75740", "#A5FFD2", "#FFB167", "#009BFF", "#E85EBE",
};

[[nodiscard]] QColor generate_color(const ::ColorSet& used_colors)
{
  spdlog::info("generate color with {}", used_colors.size());
  auto remaining_colors_view = colors | std::views::filter([used_colors](const auto& name) {
                                 return !used_colors.contains(QColor::fromString(name));
                               });

  const auto remaining_colors = remaining_colors_view.empty()
                                    ? std::vector(colors.begin(), colors.end())
                                    : std::vector(remaining_colors_view.begin(), remaining_colors_view.end());

  static std::random_device rd;
  static std::mt19937 engine(rd());
  std::uniform_int_distribution<std::size_t> dist(0, remaining_colors.size() - 1);
  const auto i = dist(engine);
  spdlog::info("Chosing color {}: {}", i, remaining_colors.at(i));
  return QColor::fromString(remaining_colors.at(i));
}

}  // namespace

ProjectModel::ProjectModel()
  : m_empty_project(add(std::make_unique<Project>(Project::Type::Empty, QString{}, Qt::gray)))
  , m_holiday_project(add(std::make_unique<Project>(Project::Type::Holiday, QString{}, Qt::darkRed)))
  , m_sick_project(add(std::make_unique<Project>(Project::Type::Sick, QString{}, Qt::green)))
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
  if (!project->color().isValid()) {
    project->set_color(generate_color());
    spdlog::info("Project {} has not color. Assigning {}.", project->label(), project->color().name());
  }
  auto& ref = *m_projects.emplace_back(std::move(project));
  Q_EMIT projects_changed();
  return ref;
}

std::unique_ptr<Project> ProjectModel::extract(const Project& project)
{
  const auto it = std::ranges::find(m_projects, &project, &std::unique_ptr<Project>::get);
  auto extracted_project = std::move(*it);
  m_projects.erase(it);
  Q_EMIT projects_changed();
  return extracted_project;
}

const Project& ProjectModel::empty_project() const noexcept
{
  return m_empty_project;
}

bool ProjectModel::is_special_project(const Project* const project) const noexcept
{
  return project == &m_empty_project || project == &m_holiday_project || project == &m_sick_project;
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

QColor ProjectModel::generate_color() const
{
  auto view = m_projects | std::views::transform(&Project::color);
  return ::generate_color(::ColorSet(view.begin(), view.end()));
}
