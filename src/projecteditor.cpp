#include "projecteditor.h"
#include "exceptions.h"
#include "intervalmodel.h"
#include "project.h"
#include "projectmodel.h"
#include "ui_projecteditor.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <ranges>

ProjectEditor::ProjectEditor(ProjectModel& model, QWidget* parent)
  : QDialog(parent), m_ui(std::make_unique<Ui::ProjectEditor>()), m_project_model(model)
{
  m_ui->setupUi(this);

  connect(m_ui->cb_type, &QComboBox::currentIndexChanged, this, &ProjectEditor::update_enabledness);
  connect(m_ui->cb_name, &QComboBox::currentTextChanged, this, &ProjectEditor::update_enabledness);

  auto labels = m_project_model.projects() | std::views::transform(&Project::label);
  m_ui->cb_name->addItems(QStringList(labels.begin(), labels.end()));
  connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
    if (current_project() == nullptr) {
      if (QMessageBox::question(
              this, QApplication::applicationDisplayName(),
              tr("There is no project '%1'. Do you want to create it?").arg(m_ui->cb_name->currentText()),
              QMessageBox::Yes | QMessageBox::No)
          == QMessageBox::No)
      {
        reject();
      } else {
        m_project_model.add_project(create_project());
      }
    }
    accept();
  });
}

ProjectEditor::~ProjectEditor() = default;

void ProjectEditor::set_project(const Project& project)
{
  m_ui->cb_type->setCurrentIndex(static_cast<int>(project.type()));
  m_ui->cb_name->setCurrentText(project.name());
}

Project* ProjectEditor::current_project() const
{
  const auto projects = m_project_model.projects();

  const auto pred = [type = current_type(), name = m_ui->cb_name->currentText()](const auto* const project) {
    return project->type() == type && (type != Project::Type::Work || project->name() == name);
  };
  if (const auto it = std::ranges::find_if(projects, pred); it != projects.end()) {
    return *it;
  }
  return nullptr;
}

std::unique_ptr<Project> ProjectEditor::create_project() const
{
  // only work items can be created.
  if (const auto type = current_type(); type == Project::Type::Work) {
    return std::make_unique<Project>(type, m_ui->cb_name->currentText());
  }
  return nullptr;
}

void ProjectEditor::update_enabledness()
{
  m_ui->cb_name->setEnabled(current_type() == Project::Type::Work);
  m_ui->buttonBox->button(QDialogButtonBox::Ok)
      ->setEnabled(!m_ui->cb_name->isEnabled() || !m_ui->cb_name->currentText().isEmpty());
}

Project::Type ProjectEditor::current_type() const
{
  return static_cast<Project::Type>(m_ui->cb_type->currentIndex());
}
