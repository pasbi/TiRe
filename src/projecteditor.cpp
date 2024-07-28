#include "projecteditor.h"

#include "exceptions.h"
#include "model.h"
#include "project.h"
#include "ui_projecteditor.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <ranges>

namespace
{
constexpr auto none_type_index = 0;
constexpr auto work_type_index = 1;
constexpr auto holiday_type_index = 2;
constexpr auto sick_type_index = 3;

[[nodiscard]] Project::Type to_enum(const int i)
{
  switch (i) {
    using enum Project::Type;
  case ::work_type_index:
    return Work;
  case ::holiday_type_index:
    return Holiday;
  case ::sick_type_index:
    return Sick;
  default:
    throw InvalidEnumNameException("Failed to convert index {} to Project::Type.", i);
  }
}

[[nodiscard]] int from_enum(const Project::Type t)
{
  switch (t) {
    using enum Project::Type;
  case Work:
    return ::work_type_index;
  case Holiday:
    return ::holiday_type_index;
  case Sick:
    return ::sick_type_index;
  }
  throw InvalidEnumNameException("Failed to convert enum {} to int.", static_cast<int>(t));
}

}  // namespace

ProjectEditor::ProjectEditor(Model& model, QWidget* parent)
  : QDialog(parent), m_ui(std::make_unique<Ui::ProjectEditor>()), m_model(model)
{
  m_ui->setupUi(this);

  connect(m_ui->cb_type, &QComboBox::currentIndexChanged, this, &ProjectEditor::update_enabledness);
  connect(m_ui->cb_name, &QComboBox::currentTextChanged, this, &ProjectEditor::update_enabledness);

  auto labels = m_model.projects() | std::views::transform([](const Project* project) { return ::label(project); });
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
        m_model.add_project(create_project());
      }
    }
    accept();
  });
}

ProjectEditor::~ProjectEditor() = default;

void ProjectEditor::set_project(const Project* const project)
{
  if (project == nullptr) {
    m_ui->cb_type->setCurrentIndex(::none_type_index);
    m_ui->cb_name->clear();
  } else {
    m_ui->cb_type->setCurrentIndex(::from_enum(project->type()));
    m_ui->cb_name->setCurrentText(project->name());
  }
}

std::optional<Project*> ProjectEditor::current_project() const
{
  const auto type_index = m_ui->cb_type->currentIndex();
  if (type_index == ::none_type_index) {
    return nullptr;
  }

  const auto projects = m_model.projects();

  const auto pred = [type = ::to_enum(type_index), name = m_ui->cb_name->currentText()](const auto* const project) {
    return project->type() == type && (type != Project::Type::Work || project->name() == name);
  };
  if (const auto it = std::ranges::find_if(projects, pred); it != projects.end()) {
    return *it;
  }
  return std::nullopt;
}

std::unique_ptr<Project> ProjectEditor::create_project() const
{
  const auto type_index = m_ui->cb_type->currentIndex();
  if (type_index == ::none_type_index) {
    return nullptr;
  }

  return std::make_unique<Project>(::to_enum(type_index), m_ui->cb_name->currentText());
}

void ProjectEditor::update_enabledness()
{
  m_ui->cb_name->setEnabled(m_ui->cb_type->currentIndex() == work_type_index);
  m_ui->buttonBox->button(QDialogButtonBox::Ok)
      ->setEnabled(!m_ui->cb_name->isEnabled() || !m_ui->cb_name->currentText().isEmpty());
}
