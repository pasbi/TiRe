#include "projecteditor.h"
#include "ui_projecteditor.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

ProjectEditor::ProjectEditor(const QStringList& projects, QWidget* parent)
  : QDialog(parent), m_ui(std::make_unique<Ui::ProjectEditor>()), m_projects(projects)
{
  m_ui->setupUi(this);
  m_ui->comboBox->addItems(projects);
  connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
    if (const auto current_text = m_ui->comboBox->currentText();
        !m_projects.contains(current_text)
        && QMessageBox::question(this, QApplication::applicationDisplayName(),
                                 tr("There is no project '%1'. Do you want to create it?").arg(current_text),
                                 QMessageBox::Yes | QMessageBox::No)
               == QMessageBox::No)
    {
      return;
    }
    accept();
  });
  if (!m_projects.empty()) {
    m_ui->comboBox->setCurrentText(m_projects.front());
  }
  connect(m_ui->comboBox->lineEdit(), &QLineEdit::textChanged, this,
          [this](const QString& text) { m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty()); });
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
