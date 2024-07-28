#include "projecteditor.h"
#include "project.h"
#include "ui_projecteditor.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <ranges>

ProjectEditor::ProjectEditor(const std::vector<Project*>& projects, QWidget* parent)
  : QDialog(parent), m_ui(std::make_unique<Ui::ProjectEditor>()), m_projects(projects)
{
  m_ui->setupUi(this);
  auto labels = projects | std::views::transform([](const Project* project) { return ::label(project); });
  m_ui->comboBox->addItems(QStringList(labels.begin(), labels.end()));
  connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
    if (const auto current_text = m_ui->comboBox->currentText();
        // !m_projects.contains(current_text)
        true  // TODO
        && QMessageBox::question(this, QApplication::applicationDisplayName(),
                                 tr("There is no project '%1'. Do you want to create it?").arg(current_text),
                                 QMessageBox::Yes | QMessageBox::No)
               == QMessageBox::No)
    {
      return;
    }
    accept();
  });
  if (!m_projects.empty()) {  // TODO m_projects cannot be empty
    m_ui->comboBox->setCurrentText(::label(m_projects.front()));
  }
}

ProjectEditor::~ProjectEditor() = default;

QString ProjectEditor::project_name() const
{
  return m_ui->comboBox->currentText();
}

void ProjectEditor::set_project(const QString& project)
{
  m_ui->comboBox->setCurrentText(project);
}
