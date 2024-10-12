#pragma once

#include "project.h"
#include <QDialog>
#include <memory>

class UndoStack;
class Project;
class ProjectModel;
namespace Ui
{
class ProjectEditor;
}

class ProjectEditor final : public QDialog
{
  Q_OBJECT

public:
  explicit ProjectEditor(ProjectModel& project_model, QWidget* parent = nullptr);
  ~ProjectEditor() override;
  void set_project(const Project& project);

  [[nodiscard]] const Project& current_project() const;
  [[nodiscard]] std::unique_ptr<Project> create_project() const;

protected:
  void showEvent(QShowEvent* event) override;

private:
  std::unique_ptr<Ui::ProjectEditor> m_ui;
  ProjectModel& m_project_model;
  void update_enabledness() const;
  void update_project_list() const;
  void about_to_accept();
};
