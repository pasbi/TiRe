#pragma once

#include <QDialog>
#include <memory>

namespace Ui
{
class ProjectEditor;
}

class ProjectEditor : public QDialog
{
  Q_OBJECT

public:
  explicit ProjectEditor(const QStringList& projects, QWidget* parent = nullptr);
  ~ProjectEditor() override;
  [[nodiscard]] QString project_name() const;
  void set_project(const QString& project);

private:
  const QStringList& m_projects;
  std::unique_ptr<Ui::ProjectEditor> m_ui;
};
