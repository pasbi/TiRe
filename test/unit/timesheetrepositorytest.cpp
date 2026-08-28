#include "db/sqltimesheetrepository.h"

#include "db/database.h"
#include "exceptions.h"
#include "interval.h"
#include "intervalmodel.h"
#include "plan.h"
#include "project.h"
#include "projectmodel.h"
#include "qtfixture.h"
#include "timesheet.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QVariant>
#include <gtest/gtest.h>

namespace
{

using std::chrono_literals::operator""min;

class TimeSheetRepositoryTest : public QtFixture
{
protected:
  void SetUp() override
  {
    m_database = std::make_unique<Database>(Database::open_in_memory());
    m_database->migrate();
    m_repository = std::make_unique<SqlTimeSheetRepository>(*m_database);
  }

  [[nodiscard]] Project& add_project(const TimeSheet& time_sheet, const QString& name, const QColor& color) const
  {
    return time_sheet.project_model().add(std::make_unique<Project>(name, color));
  }

  [[nodiscard]] static Interval& add_interval(const TimeSheet& time_sheet, const Project* project,
                                              const QDateTime& begin, const QDateTime& end)
  {
    auto interval = std::make_unique<Interval>(project);
    interval->swap_begin(begin);
    interval->swap_end(end);
    time_sheet.interval_model().add(std::move(interval));
    return time_sheet.interval_model().remove_const(
        *time_sheet.interval_model().interval(time_sheet.interval_model().intervals().size() - 1));
  }

  std::unique_ptr<Database> m_database;
  std::unique_ptr<SqlTimeSheetRepository> m_repository;
};

}  // namespace

TEST_F(TimeSheetRepositoryTest, RoundTripEmpty)
{
  const auto time_sheet = m_repository->load();
  EXPECT_TRUE(time_sheet->project_model().projects().empty());
  EXPECT_TRUE(time_sheet->interval_model().intervals().empty());
  EXPECT_EQ(0, time_sheet->plan().rowCount({}));
  EXPECT_EQ(0min, time_sheet->plan().overtime_offset());
  // A fresh database must get its plan settings row, or every launch would silently re-pin the
  // plan start date to "today" and overtime would never accumulate.
  EXPECT_TRUE(time_sheet->plan().start().isValid());
}

TEST_F(TimeSheetRepositoryTest, ExtractAndReAddKeepsId)
{
  // The load-bearing test for putting the id on the entity rather than in a repository-side map:
  // a command holds an extracted object alive and puts the *same* object back on undo, so the
  // row -- and every foreign key pointing at it -- must survive the round trip.
  const auto time_sheet = m_repository->load();
  auto& project = add_project(*time_sheet, QStringLiteral("recurring"), QColor{1, 2, 3});
  const auto original_id = project.id();
  ASSERT_TRUE(original_id.is_valid());

  add_interval(*time_sheet, &project, QDateTime{QDate{2026, 3, 1}, QTime{8, 0}},
               QDateTime{QDate{2026, 3, 1}, QTime{16, 0}});

  // Removing the project requires removing the referencing interval first (ON DELETE RESTRICT).
  auto owned_interval = time_sheet->interval_model().extract(*time_sheet->interval_model().intervals().front());
  auto owned_project = time_sheet->project_model().extract(project);
  EXPECT_EQ(original_id, owned_project->id());

  // ... and this is the undo.
  auto& re_added = time_sheet->project_model().add(std::move(owned_project));
  time_sheet->interval_model().add(std::move(owned_interval));
  EXPECT_EQ(original_id, re_added.id());

  const auto reloaded = m_repository->load();
  ASSERT_EQ(1U, reloaded->project_model().projects().size());
  EXPECT_EQ(original_id, reloaded->project_model().projects().front()->id());
  ASSERT_EQ(1U, reloaded->interval_model().intervals().size());
  EXPECT_EQ(reloaded->project_model().projects().front(), reloaded->interval_model().intervals().front()->project());
}

TEST_F(TimeSheetRepositoryTest, CreatingTheFirstProjectWorks)
{
  // Regression: the project editor used to decide what the typed text meant from the combo box's
  // row index. With no projects yet, index 0 is the "No Project" sentinel, so the very first
  // project could never be created -- the typed name was silently discarded. Name lookup has to
  // work on an empty model.
  const auto time_sheet = m_repository->load();
  auto& projects = time_sheet->project_model();
  ASSERT_TRUE(projects.projects().empty());
  EXPECT_EQ(nullptr, projects.find(QStringLiteral("brand new")));

  auto& created = add_project(*time_sheet, QStringLiteral("brand new"), QColor{1, 2, 3});
  EXPECT_EQ(&created, projects.find(QStringLiteral("brand new")));
  EXPECT_EQ(nullptr, projects.find(QStringLiteral("still missing")));
  // The sentinel label must never resolve to a real project.
  EXPECT_EQ(nullptr, projects.find(QStringLiteral("No Project")));
  EXPECT_EQ(nullptr, projects.find(QString{}));

  const auto reloaded = m_repository->load();
  ASSERT_EQ(1U, reloaded->project_model().projects().size());
  EXPECT_EQ(QStringLiteral("brand new"), reloaded->project_model().projects().front()->name());
}

TEST_F(TimeSheetRepositoryTest, RoundTripFull)
{
  const auto time_sheet = m_repository->load();
  auto& alpha = add_project(*time_sheet, QStringLiteral("alpha"), QColor{10, 20, 30, 128});
  auto& beta = add_project(*time_sheet, QStringLiteral("beta"), QColor{200, 100, 50});

  add_interval(*time_sheet, &alpha, QDateTime{QDate{2026, 3, 1}, QTime{8, 0}},
               QDateTime{QDate{2026, 3, 1}, QTime{16, 30}});
  add_interval(*time_sheet, &beta, QDateTime{QDate{2026, 3, 2}, QTime{9, 15}},
               QDateTime{QDate{2026, 3, 2}, QTime{17, 45}});
  add_interval(*time_sheet, nullptr, QDateTime{QDate{2026, 3, 3}, QTime{7, 0}},
               QDateTime{QDate{2026, 3, 3}, QTime{8, 0}});
  add_interval(*time_sheet, &alpha, QDateTime{QDate{2026, 3, 4}, QTime{10, 0}}, {});

  const auto reloaded = m_repository->load();

  ASSERT_EQ(2U, reloaded->project_model().projects().size());
  EXPECT_EQ(QStringLiteral("alpha"), reloaded->project_model().projects().at(0)->name());
  EXPECT_EQ(QColor(10, 20, 30, 128), reloaded->project_model().projects().at(0)->color());
  EXPECT_EQ(128, reloaded->project_model().projects().at(0)->color().alpha());

  const auto intervals = reloaded->interval_model().intervals();
  ASSERT_EQ(4U, intervals.size());
  EXPECT_EQ(QDateTime(QDate(2026, 3, 1), QTime(8, 0)), intervals.at(0)->begin());
  EXPECT_EQ(QDateTime(QDate(2026, 3, 1), QTime(16, 30)), intervals.at(0)->end());
}

TEST_F(TimeSheetRepositoryTest, ProjectPointersAreRebuilt)
{
  const auto time_sheet = m_repository->load();
  auto& alpha = add_project(*time_sheet, QStringLiteral("alpha"), QColor{10, 20, 30});
  auto& beta = add_project(*time_sheet, QStringLiteral("beta"), QColor{40, 50, 60});
  add_interval(*time_sheet, &beta, QDateTime{QDate{2026, 3, 1}, QTime{8, 0}},
               QDateTime{QDate{2026, 3, 1}, QTime{9, 0}});
  add_interval(*time_sheet, nullptr, QDateTime{QDate{2026, 3, 2}, QTime{8, 0}},
               QDateTime{QDate{2026, 3, 2}, QTime{9, 0}});
  Q_UNUSED(alpha)

  const auto reloaded = m_repository->load();
  const auto* const reloaded_beta = reloaded->project_model().projects().at(1);
  ASSERT_EQ(QStringLiteral("beta"), reloaded_beta->name());
  // Identity, not just equality: the interval must point at the very object the model owns.
  EXPECT_EQ(reloaded_beta, reloaded->interval_model().intervals().at(0)->project());
  EXPECT_EQ(nullptr, reloaded->interval_model().intervals().at(1)->project());
}

TEST_F(TimeSheetRepositoryTest, OngoingIntervalStaysOngoing)
{
  const auto time_sheet = m_repository->load();
  auto& project = add_project(*time_sheet, QStringLiteral("running"), QColor{1, 2, 3});
  add_interval(*time_sheet, &project, QDateTime{QDate{2026, 3, 4}, QTime{10, 0}}, {});

  const auto reloaded = m_repository->load();
  ASSERT_EQ(1U, reloaded->interval_model().intervals().size());
  EXPECT_FALSE(reloaded->interval_model().intervals().front()->end().isValid());
  EXPECT_EQ(1U, reloaded->interval_model().open_intervals().size());
}

TEST_F(TimeSheetRepositoryTest, PeriodTypeSurvivesRoundTrip)
{
  const auto time_sheet = m_repository->load();
  time_sheet->plan().add(
      std::make_unique<Plan::Entry>(Period{QDate{2026, 5, 4}, Period::Type::Month}, Plan::Kind::Vacation, EntityId{}));
  time_sheet->plan().add(std::make_unique<Plan::Entry>(Period{QDate{2026, 8, 3}, QDate{2026, 8, 7}},
                                                       Plan::Kind::HalfVacationHalfHoliday, EntityId{}));

  const auto reloaded = m_repository->load();
  ASSERT_EQ(2, reloaded->plan().rowCount({}));

  const auto& monthly = reloaded->plan().entry(0);
  EXPECT_EQ(Period::Type::Month, monthly.period.type());
  EXPECT_EQ(QDate(2026, 5, 1), monthly.period.begin());
  EXPECT_EQ(QDate(2026, 5, 31), monthly.period.end());
  EXPECT_EQ(Plan::Kind::Vacation, monthly.kind);

  const auto& custom = reloaded->plan().entry(1);
  EXPECT_EQ(Period::Type::Custom, custom.period.type());
  EXPECT_EQ(QDate(2026, 8, 3), custom.period.begin());
  EXPECT_EQ(QDate(2026, 8, 7), custom.period.end());
  EXPECT_EQ(Plan::Kind::HalfVacationHalfHoliday, custom.kind);
}

TEST_F(TimeSheetRepositoryTest, EveryPlanKindSurvivesRoundTrip)
{
  const auto time_sheet = m_repository->load();
  using enum Plan::Kind;
  constexpr auto kinds =
      std::array{Normal, Sick, Holiday, HalfHoliday, Vacation, HalfVacation, HalfVacationHalfHoliday};
  for (auto i = 0U; i < kinds.size(); ++i) {
    const auto day = QDate{2026, 6, static_cast<int>(i) + 1};
    time_sheet->plan().add(std::make_unique<Plan::Entry>(Period{day, day}, kinds.at(i), EntityId{}));
  }

  const auto reloaded = m_repository->load();
  ASSERT_EQ(static_cast<int>(kinds.size()), reloaded->plan().rowCount({}));
  for (auto i = 0U; i < kinds.size(); ++i) {
    EXPECT_EQ(kinds.at(i), reloaded->plan().entry(static_cast<int>(i)).kind);
  }
}

TEST_F(TimeSheetRepositoryTest, PlanSettingsSurviveRoundTrip)
{
  const auto time_sheet = m_repository->load();
  time_sheet->plan().swap_start(QDate{2024, 1, 15});
  time_sheet->plan().swap_overtime_offset(-90min);

  const auto reloaded = m_repository->load();
  EXPECT_EQ(QDate(2024, 1, 15), reloaded->plan().start());
  EXPECT_EQ(-90min, reloaded->plan().overtime_offset());
}

TEST_F(TimeSheetRepositoryTest, PlanEntryOverlapIsRejectedOnLoad)
{
  auto insert_entry = [this](const int id, const QString& begin, const QString& end) {
    auto query = m_database->prepare(QStringLiteral(
        "INSERT INTO plan_entry (id, period_begin, period_end, period_type, kind) VALUES (?, ?, ?, 'CUSTOM', 'SICK')"));
    query.addBindValue(id);
    query.addBindValue(begin);
    query.addBindValue(end);
    m_database->execute(query);
  };
  insert_entry(1, QStringLiteral("2026-01-01"), QStringLiteral("2026-01-10"));
  insert_entry(2, QStringLiteral("2026-01-05"), QStringLiteral("2026-01-15"));

  EXPECT_THROW(m_repository->load(), DatabaseError);
}

TEST_F(TimeSheetRepositoryTest, ModelsWorkWithoutDatabase)
{
  // The null-repository escape hatch: models must stay usable with no database at all, which is
  // what keeps plantest and the benchmarks free of SQL.
  const TimeSheet time_sheet;
  EXPECT_NO_THROW({
    auto& project = time_sheet.project_model().add(std::make_unique<Project>(QStringLiteral("p"), QColor{1, 2, 3}));
    auto interval = std::make_unique<Interval>(&project);
    interval->swap_begin(QDateTime{QDate{2026, 3, 1}, QTime{8, 0}});
    time_sheet.interval_model().add(std::move(interval));
    time_sheet.plan().add(
        std::make_unique<Plan::Entry>(Period{QDate{2026, 3, 5}, QDate{2026, 3, 6}}, Plan::Kind::Sick, EntityId{}));
    time_sheet.plan().swap_overtime_offset(15min);
  });
  EXPECT_EQ(1U, time_sheet.project_model().projects().size());
  EXPECT_EQ(1U, time_sheet.interval_model().intervals().size());
}

TEST_F(TimeSheetRepositoryTest, ReopenPersistsToDisk)
{
  // The only test that proves the durability story end to end: write, drop the connection
  // entirely, reopen the file, and expect everything back.
  const QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const auto path = std::filesystem::path{directory.filePath(QStringLiteral("tire.db")).toStdString()};

  {
    auto database = Database::open_file(path);
    database.migrate();
    SqlTimeSheetRepository repository{database};
    const auto time_sheet = repository.load();
    auto& project =
        time_sheet->project_model().add(std::make_unique<Project>(QStringLiteral("persisted"), QColor{9, 8, 7}));
    auto interval = std::make_unique<Interval>(&project);
    interval->swap_begin(QDateTime{QDate{2026, 4, 1}, QTime{9, 0}});
    interval->swap_end(QDateTime{QDate{2026, 4, 1}, QTime{17, 0}});
    time_sheet->interval_model().add(std::move(interval));
    time_sheet->plan().swap_overtime_offset(42min);
  }

  {
    auto database = Database::open_file(path);
    database.migrate();
    SqlTimeSheetRepository repository{database};
    const auto time_sheet = repository.load();
    ASSERT_EQ(1U, time_sheet->project_model().projects().size());
    EXPECT_EQ(QStringLiteral("persisted"), time_sheet->project_model().projects().front()->name());
    ASSERT_EQ(1U, time_sheet->interval_model().intervals().size());
    EXPECT_EQ(QDateTime(QDate(2026, 4, 1), QTime(17, 0)), time_sheet->interval_model().intervals().front()->end());
    EXPECT_EQ(42min, time_sheet->plan().overtime_offset());
  }
}

TEST_F(TimeSheetRepositoryTest, NewIdsDoNotCollideAfterReload)
{
  {
    const auto time_sheet = m_repository->load();
    add_project(*time_sheet, QStringLiteral("first"), QColor{1, 2, 3});
    add_project(*time_sheet, QStringLiteral("second"), QColor{4, 5, 6});
  }
  // A repository constructed against an already-populated database must continue the id sequence
  // rather than restart it, or the next insert collides with an existing primary key.
  SqlTimeSheetRepository fresh_repository{*m_database};
  const auto time_sheet = fresh_repository.load();
  EXPECT_NO_THROW(time_sheet->project_model().add(std::make_unique<Project>(QStringLiteral("third"), QColor{7, 8, 9})));
  EXPECT_EQ(3U, fresh_repository.load()->project_model().projects().size());
}
