#pragma once
#include "project.h"

#include <memory>
#include <vector>

class Project;
class ProjectModel : public QObject
{
  Q_OBJECT
public:
  explicit ProjectModel();
  explicit ProjectModel(std::vector<std::unique_ptr<Project>> projects);
  ~ProjectModel();

  [[nodiscard]] std::vector<Project*> projects() const;
  Project& add(std::unique_ptr<Project> project);
  std::unique_ptr<Project> extract(const Project& project);
  [[nodiscard]] const Project& empty_project() const noexcept;
  [[nodiscard]] bool is_special_project(const Project* project) const noexcept;
  [[nodiscard]] const Project& project(std::size_t index) const;
  [[nodiscard]] std::size_t index_of(const Project& project) const;
  [[nodiscard]] QColor generate_color() const;

Q_SIGNALS:
  void projects_changed();

private:
  std::vector<std::unique_ptr<Project>> m_projects;
  const Project& m_empty_project;
  const Project& m_holiday_project;
  const Project& m_sick_project;
};
