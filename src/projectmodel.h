#pragma once
#include "project.h"

#include <memory>
#include <vector>

class AbstractTimeSheetRepository;
class Project;
class ProjectModel : public QObject
{
  Q_OBJECT
public:
  /** @brief Creates a model that does not persist anything. */
  explicit ProjectModel();
  explicit ProjectModel(AbstractTimeSheetRepository& repository);
  /** @brief Adopts already-stored projects without writing them back. */
  explicit ProjectModel(AbstractTimeSheetRepository& repository, std::vector<std::unique_ptr<Project>> projects);
  ~ProjectModel() override;

  [[nodiscard]] std::vector<Project*> projects() const;
  Project& add(std::unique_ptr<Project> project);
  std::unique_ptr<Project> extract(const Project& project);
  [[nodiscard]] const Project& project(std::size_t index) const;

  /**
   * @brief The project called @p name, or nullptr if there is none.
   * Name lookup is what the project editor needs: an editable combo box has no usable index for
   * text the user just typed, and no index at all to distinguish "no project" from "not created
   * yet".
   */
  [[nodiscard]] const Project* find(const QString& name) const;
  [[nodiscard]] std::size_t index_of(const Project& project) const;
  [[nodiscard]] QColor generate_color() const;

Q_SIGNALS:
  void projects_changed();

private:
  AbstractTimeSheetRepository& m_repository;
  std::vector<std::unique_ptr<Project>> m_projects;
};
