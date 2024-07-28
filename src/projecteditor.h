#pragma once

#include <QDialog>
#include <memory>

class Project;
namespace Ui
{
class ProjectEditor;
}

class ProjectEditor : public QDialog
{
  Q_OBJECT

public:
  explicit ProjectEditor(const std::vector<Project*>& projects, QWidget* parent = nullptr);
  ~ProjectEditor() override;
  [[nodiscard]] QString project_name() const;
  void set_project(const QString& project);

private:
  const std::vector<Project*>& m_projects;
  std::unique_ptr<Ui::ProjectEditor> m_ui;
};
