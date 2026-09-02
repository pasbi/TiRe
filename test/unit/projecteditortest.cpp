#include "application.h"

#include "commands/undostack.h"
#include "db/sqltimesheetrepository.h"
#include "interval.h"
#include "intervalmodel.h"
#include "period.h"
#include "project.h"
#include "projectmodel.h"
#include "timesheet.h"
#include "views/perioddetailview.h"

#include <QAbstractItemDelegate>
#include <QApplication>
#include <QComboBox>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QTemporaryDir>
#include <QTimer>
#include <gtest/gtest.h>

/**
 * Drives the real project-column delegate, because the bug it guards against lived in how that
 * delegate turned the combo box's state into a project -- not in anything the models could see.
 */
namespace
{

/**
 * @brief Clicks a button on the next modal dialog that appears.
 * setModelData asks for confirmation before creating a project, which would block the test.
 */
void answer_next_question(const QMessageBox::StandardButton button)
{
  auto* const timer = new QTimer;
  timer->setInterval(10);
  QObject::connect(timer, &QTimer::timeout, [timer, button] {
    auto* const box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
    if (box == nullptr) {
      return;
    }
    box->button(button)->click();
    timer->stop();
    timer->deleteLater();
  });
  timer->start();
}

class ProjectEditorTest : public ::testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    static QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    static const auto db_argument =
        QStringLiteral("--database=%1").arg(directory.filePath(QStringLiteral("tire.db"))).toStdString();
    static auto arg0 = std::string{"tire-test"};
    static std::array argv{arg0.data(), const_cast<char*>(db_argument.c_str()), static_cast<char*>(nullptr)};
    static auto argc = 2;
    static auto* const application = new Application{argc, argv.data()};
    const auto result = application->open_database();
    ASSERT_TRUE(result.ok) << result.message.toStdString();
  }

  void SetUp() override
  {
    m_time_sheet = Application::sql_repository().load();
    Application::undo_stack().impl().clear();

    // One interval on a known day, so the view has a row whose project cell can be edited.
    auto interval = std::make_unique<Interval>(nullptr);
    interval->swap_begin(QDateTime{m_day, QTime{9, 0}});
    interval->swap_end(QDateTime{m_day, QTime{17, 0}});
    m_time_sheet->interval_model().add(std::move(interval));

    m_view = std::make_unique<PeriodDetailView>();
    m_view->set_model(m_time_sheet.get());
    m_view->set_period(Period{m_day, Period::Type::Day});

    m_table_view = m_view->findChild<QTableView*>();
    ASSERT_NE(nullptr, m_table_view);
  }

  void TearDown() override
  {
    m_view.reset();
    for (auto* const interval : m_time_sheet->interval_model().intervals()) {
      m_time_sheet->interval_model().extract(*interval);
    }
    for (auto* const project : m_time_sheet->project_model().projects()) {
      m_time_sheet->project_model().extract(*project);
    }
    m_time_sheet.reset();
  }

  /** @brief Types @p text into the project cell and commits it, exactly as the user does. */
  void type_project(const QString& text) const
  {
    const auto index = m_table_view->model()->index(0, project_column);
    ASSERT_TRUE(index.isValid());
    auto* const delegate = m_table_view->itemDelegateForColumn(project_column);
    ASSERT_NE(nullptr, delegate);

    auto* const editor = delegate->createEditor(m_table_view->viewport(), {}, index);
    ASSERT_NE(nullptr, editor);
    delegate->setEditorData(editor, index);
    dynamic_cast<QComboBox&>(*editor).setCurrentText(text);
    delegate->setModelData(editor, m_table_view->model(), index);
    delete editor;
  }

  static constexpr auto project_column = 0;
  const QDate m_day{2026, 4, 7};
  std::unique_ptr<TimeSheet> m_time_sheet;
  std::unique_ptr<PeriodDetailView> m_view;
  QTableView* m_table_view = nullptr;
};

}  // namespace

TEST_F(ProjectEditorTest, CreatesTheVeryFirstProject)
{
  // The reported bug: with no projects yet, typing a name did nothing and the cell cleared.
  ASSERT_TRUE(m_time_sheet->project_model().projects().empty());

  answer_next_question(QMessageBox::Yes);
  type_project(QStringLiteral("consulting"));

  ASSERT_EQ(1U, m_time_sheet->project_model().projects().size());
  EXPECT_EQ(QStringLiteral("consulting"), m_time_sheet->project_model().projects().front()->name());
  const auto* const assigned = m_time_sheet->interval_model().intervals().front()->project();
  ASSERT_NE(nullptr, assigned) << "the new project must also be assigned to the interval";
  EXPECT_EQ(QStringLiteral("consulting"), assigned->name());

  // And it must have reached the database, not just the model.
  const auto reloaded = Application::sql_repository().load();
  ASSERT_EQ(1U, reloaded->project_model().projects().size());
  EXPECT_EQ(QStringLiteral("consulting"), reloaded->project_model().projects().front()->name());
  ASSERT_NE(nullptr, reloaded->interval_model().intervals().front()->project());
}

TEST_F(ProjectEditorTest, CreatingASecondProjectStillWorks)
{
  answer_next_question(QMessageBox::Yes);
  type_project(QStringLiteral("first"));
  answer_next_question(QMessageBox::Yes);
  type_project(QStringLiteral("second"));

  EXPECT_EQ(2U, m_time_sheet->project_model().projects().size());
  EXPECT_EQ(QStringLiteral("second"), m_time_sheet->interval_model().intervals().front()->project()->name());
}

TEST_F(ProjectEditorTest, PicksAnExistingProjectWithoutAsking)
{
  answer_next_question(QMessageBox::Yes);
  type_project(QStringLiteral("existing"));
  const auto* const created = m_time_sheet->project_model().projects().front();

  // No dialog should appear this time; if one does, the test would hang, so nothing answers it.
  type_project(QStringLiteral("existing"));
  EXPECT_EQ(1U, m_time_sheet->project_model().projects().size()) << "must not create a duplicate";
  EXPECT_EQ(created, m_time_sheet->interval_model().intervals().front()->project());
}

TEST_F(ProjectEditorTest, DecliningCreatesNothing)
{
  answer_next_question(QMessageBox::No);
  type_project(QStringLiteral("rejected"));

  EXPECT_TRUE(m_time_sheet->project_model().projects().empty());
  EXPECT_EQ(nullptr, m_time_sheet->interval_model().intervals().front()->project());
}

TEST_F(ProjectEditorTest, TheNoProjectSentinelClearsTheAssignment)
{
  answer_next_question(QMessageBox::Yes);
  type_project(QStringLiteral("temporary"));
  ASSERT_NE(nullptr, m_time_sheet->interval_model().intervals().front()->project());

  // Must be understood as "no project", not as a project that needs creating.
  type_project(QObject::tr("No Project"));
  EXPECT_EQ(nullptr, m_time_sheet->interval_model().intervals().front()->project());
  EXPECT_EQ(1U, m_time_sheet->project_model().projects().size()) << "no project called 'No Project' may be created";
}
