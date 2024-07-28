#pragma once

#include <QDialog>
#include <memory>

class Project;
class Model;
namespace Ui
{
class ProjectEditor;
}

class ProjectEditor : public QDialog
{
  Q_OBJECT

public:
  explicit ProjectEditor(Model& model, QWidget* parent = nullptr);
  ~ProjectEditor() override;
  [[nodiscard]] QString project_name() const;
  void set_project(const Project* project);

  [[nodiscard]] std::optional<Project*> current_project() const;
  std::unique_ptr<Project> create_project() const;

private:
  Model& m_model;
  std::unique_ptr<Ui::ProjectEditor> m_ui;
  void update_enabledness();
};
