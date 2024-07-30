#include "projecteditor.h"
#include "project.h"
#include "projectmodel.h"
#include "ui_projecteditor.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <spdlog/spdlog.h>

ProjectEditor::ProjectEditor(ProjectModel& project_model, QWidget* parent)
  : QDialog(parent), m_ui(std::make_unique<Ui::ProjectEditor>()), m_project_model(project_model)
{
  m_ui->setupUi(this);

  update_project_list();
  connect(m_ui->cb_name, &QComboBox::currentTextChanged, this, &ProjectEditor::update_enabledness);
  connect(m_ui->cb_name, &QComboBox::currentIndexChanged, this, [](int index) {  //x
    spdlog::info("index: {}", index);
  });
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
      const auto& project = m_project_model.add(create_project());
      update_project_list();
      set_project(project);
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

void ProjectEditor::update_project_list() const
{
  const QSignalBlocker _(m_ui->cb_name);
  m_ui->cb_name->clear();
  for (const auto& project : m_project_model.projects()) {
    m_ui->cb_name->addItem(project->label());
  }
}
