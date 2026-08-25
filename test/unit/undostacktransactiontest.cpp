#include "application.h"

#include "commands/addremovecommand.h"
#include "commands/commands.h"
#include "commands/undostack.h"
#include "db/sqltimesheetrepository.h"
#include "interval.h"
#include "intervalmodel.h"
#include "plan.h"
#include "project.h"
#include "projectmodel.h"
#include "timesheet.h"

#include <QTemporaryDir>
#include <gtest/gtest.h>

/**
 * Integration test for the path a real edit takes:
 * UndoStack -> transaction -> model mutator -> repository -> database.
 *
 * The repository tests exercise the models directly; this one goes through the undo stack, which
 * is where the transaction boundaries live and where undo and redo must also write through.
 */
namespace
{

class UndoStackTransactionTest : public ::testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    // No display in CI, and none needed.
    qputenv("QT_QPA_PLATFORM", "offscreen");
    static QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    static const auto db_argument =
        QStringLiteral("--database=%1").arg(directory.filePath(QStringLiteral("tire.db"))).toStdString();
    static auto arg0 = std::string{"tire-test"};
    static std::array argv{arg0.data(), const_cast<char*>(db_argument.c_str()), static_cast<char*>(nullptr)};
    static auto argc = 2;
    // Leaked deliberately: a QApplication torn down while gtest still holds static state is not
    // worth the risk, and the process exits right after.
    static auto* const application = new Application{argc, argv.data()};
    const auto result = application->open_database();
    ASSERT_TRUE(result.ok) << result.message.toStdString();
  }

  void SetUp() override
  {
    m_time_sheet = Application::sql_repository().load();
    Application::undo_stack().impl().clear();
  }

  void TearDown() override
  {
    // Leave the database empty for the next test.
    for (auto* const interval : m_time_sheet->interval_model().intervals()) {
      m_time_sheet->interval_model().extract(*interval);
    }
    for (auto* const project : m_time_sheet->project_model().projects()) {
      m_time_sheet->project_model().extract(*project);
    }
    m_time_sheet.reset();
  }

  [[nodiscard]] static std::unique_ptr<Interval> make_interval(const Project* project, const QDateTime& begin)
  {
    auto interval = std::make_unique<Interval>(project);
    interval->swap_begin(begin);
    return interval;
  }

  /** @brief Interval count as the database sees it, not as the model does. */
  [[nodiscard]] static std::size_t stored_interval_count()
  {
    return Application::sql_repository().load()->interval_model().intervals().size();
  }

  std::unique_ptr<TimeSheet> m_time_sheet;
};

}  // namespace

TEST_F(UndoStackTransactionTest, PushWritesThrough)
{
  auto& model = m_time_sheet->interval_model();
  Application::undo_stack().push(
      make<AddCommand>(model, make_interval(nullptr, QDateTime{QDate{2026, 5, 1}, QTime{8, 0}})));
  EXPECT_EQ(1U, stored_interval_count());
}

TEST_F(UndoStackTransactionTest, UndoAndRedoWriteThrough)
{
  auto& model = m_time_sheet->interval_model();
  Application::undo_stack().push(
      make<AddCommand>(model, make_interval(nullptr, QDateTime{QDate{2026, 5, 1}, QTime{8, 0}})));
  ASSERT_EQ(1U, stored_interval_count());

  Application::undo_stack().undo();
  EXPECT_EQ(0U, stored_interval_count()) << "undo must reach the database, not just memory";

  Application::undo_stack().redo();
  EXPECT_EQ(1U, stored_interval_count()) << "redo must reach the database, not just memory";
}

TEST_F(UndoStackTransactionTest, ModifyingAnOwnedIntervalWritesThrough)
{
  auto& model = m_time_sheet->interval_model();
  Application::undo_stack().push(
      make<AddCommand>(model, make_interval(nullptr, QDateTime{QDate{2026, 5, 1}, QTime{8, 0}})));
  const auto end = QDateTime{QDate{2026, 5, 1}, QTime{17, 0}};
  Application::undo_stack().push(
      make_modify_interval_command(model, *model.intervals().front(), end, &Interval::swap_end));

  EXPECT_EQ(end, Application::sql_repository().load()->interval_model().intervals().front()->end());

  Application::undo_stack().undo();
  EXPECT_FALSE(Application::sql_repository().load()->interval_model().intervals().front()->end().isValid())
      << "undoing a swap_end must clear the stored end too";
}

TEST_F(UndoStackTransactionTest, RetimingAnIntervalLaterWritesThrough)
{
  // Reproduces the ordinary "move this interval to a later time" edit. PeriodDetailView pushes
  // swap_begin and swap_end as two separate commands, so the row passes through an intermediate
  // state whose begin is after its end. The store must tolerate that, because the in-memory model
  // does.
  auto& model = m_time_sheet->interval_model();
  Application::undo_stack().push(
      make<AddCommand>(model, make_interval(nullptr, QDateTime{QDate{2026, 5, 1}, QTime{8, 0}})));
  Application::undo_stack().push(make_modify_interval_command(
      model, *model.intervals().front(), QDateTime{QDate{2026, 5, 1}, QTime{9, 0}}, &Interval::swap_end));
  ASSERT_EQ(1U, stored_interval_count());

  const auto new_begin = QDateTime{QDate{2026, 5, 1}, QTime{10, 0}};
  const auto new_end = QDateTime{QDate{2026, 5, 1}, QTime{11, 0}};
  {
    const auto macro = Application::undo_stack().start_macro(QStringLiteral("Change interval"));
    Application::undo_stack().push(
        make_modify_interval_command(model, *model.intervals().front(), new_begin, &Interval::swap_begin));
    Application::undo_stack().push(
        make_modify_interval_command(model, *model.intervals().front(), new_end, &Interval::swap_end));
  }

  const auto reloaded = Application::sql_repository().load();
  const auto stored = reloaded->interval_model().intervals();
  ASSERT_EQ(1U, stored.size());
  EXPECT_EQ(new_begin.toString(Qt::ISODate), stored.front()->begin().toString(Qt::ISODate));
  EXPECT_EQ(new_end.toString(Qt::ISODate), stored.front()->end().toString(Qt::ISODate));
  EXPECT_EQ(new_begin.toString(Qt::ISODate), model.intervals().front()->begin().toString(Qt::ISODate))
      << "in-memory value";
}

TEST_F(UndoStackTransactionTest, MacroIsOneUndoStepAndLandsCompletely)
{
  auto& model = m_time_sheet->interval_model();
  {
    const auto macro = Application::undo_stack().start_macro(QStringLiteral("add three"));
    for (auto day = 1; day <= 3; ++day) {
      Application::undo_stack().push(
          make<AddCommand>(model, make_interval(nullptr, QDateTime{QDate{2026, 5, day}, QTime{8, 0}})));
    }
  }
  EXPECT_EQ(3U, stored_interval_count());

  // One macro is one undo step, and it must roll the whole group back in the database as well.
  Application::undo_stack().undo();
  EXPECT_EQ(0U, stored_interval_count());

  Application::undo_stack().redo();
  EXPECT_EQ(3U, stored_interval_count());
}

TEST_F(UndoStackTransactionTest, PlanEditsAreUndoableAndPersisted)
{
  auto& plan = m_time_sheet->plan();
  plan.add(std::make_unique<Plan::Entry>(Period{QDate{2026, 7, 1}, QDate{2026, 7, 3}}, Plan::Kind::Normal, EntityId{}));
  ASSERT_EQ(1, Application::sql_repository().load()->plan().rowCount({}));

  Application::undo_stack().push(make_modify_plan_kind_command(plan, plan.entry(0), Plan::Kind::Sick));
  EXPECT_EQ(Plan::Kind::Sick, Application::sql_repository().load()->plan().entry(0).kind);

  // Plan edits used to bypass the undo stack entirely; they must not any more.
  Application::undo_stack().undo();
  EXPECT_EQ(Plan::Kind::Normal, Application::sql_repository().load()->plan().entry(0).kind);

  for (auto* const entry_owner = &plan.entry(0); entry_owner != nullptr;) {
    plan.extract(*entry_owner);
    break;
  }
}

TEST_F(UndoStackTransactionTest, PlanSettingsArePersistedAndUndoable)
{
  auto& plan = m_time_sheet->plan();
  const auto original = plan.start();
  Application::undo_stack().push(make_modify_plan_start_command(plan, QDate{2023, 2, 1}));
  EXPECT_EQ(QDate(2023, 2, 1), Application::sql_repository().load()->plan().start());

  Application::undo_stack().undo();
  EXPECT_EQ(original, Application::sql_repository().load()->plan().start());
}
