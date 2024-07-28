#include "projecteditor.h"
#include "project.h"
#include "projectmodel.h"
#include "ui_projecteditor.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <ranges>
#include <spdlog/spdlog.h>

ProjectEditor::ProjectEditor(ProjectModel& project_model, QWidget* parent)
  : QDialog(parent), m_ui(std::make_unique<Ui::ProjectEditor>()), m_project_model(project_model)
{
  m_ui->setupUi(this);

  // connect(m_ui->cb_type, &QComboBox::currentIndexChanged, this, &ProjectEditor::update_enabledness);
  connect(m_ui->cb_name, &QComboBox::currentTextChanged, this, &ProjectEditor::update_enabledness);

  auto labels = m_project_model.projects() | std::views::transform(&Project::label);
  m_ui->cb_name->addItems(QStringList(labels.begin(), labels.end()));
  connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
    if (m_ui->cb_name->currentText() != m_ui->cb_name->itemText(m_ui->cb_name->currentIndex())) {
      if (QMessageBox::question(
              this, QApplication::applicationDisplayName(),
              tr("There is no project '%1'. Do you want to create it?").arg(m_ui->cb_name->currentText()),
              QMessageBox::Yes | QMessageBox::No)
          == QMessageBox::No)
      {
        return;
      }
      m_project_model.add_project(create_project());
      m_ui->cb_name->setCurrentIndex(static_cast<int>(m_project_model.projects().size()));
    }
    accept();
  });
}

ProjectEditor::~ProjectEditor() = default;

void ProjectEditor::set_project(const Project& project)
{

  m_ui->cb_name->setCurrentIndex(m_project_model.index_of(project));
}

const Project& ProjectEditor::current_project() const
{
  return m_project_model.project(m_ui->cb_name->currentIndex());
}

std::unique_ptr<Project> ProjectEditor::create_project() const
{
  return std::make_unique<Project>(Project::Type::Work, m_ui->cb_name->currentText());
}

void ProjectEditor::update_enabledness() const
{
  m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!m_ui->cb_name->currentText().isEmpty());
}
