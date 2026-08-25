#include "db/sqltimesheetrepository.h"

#include "application.h"
#include "db/database.h"
#include "db/enumnames.h"
#include "db/sqlvalue.h"
#include "exceptions.h"
#include "interval.h"
#include "intervalmodel.h"
#include "plan.h"
#include "project.h"
#include "projectmodel.h"
#include "timesheet.h"

#include <QSqlQuery>
#include <QVariant>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace
{

constexpr auto plan_setting_id = 1;

[[nodiscard]] QVariant id_to_sql(const EntityId id)
{
  return QVariant::fromValue(static_cast<qlonglong>(id.value()));
}

/** @brief The id of the project an interval points at, or null when it has none. */
[[nodiscard]] QVariant project_reference(const Interval& interval)
{
  if (interval.project() == nullptr) {
    return {};
  }
  return ::id_to_sql(interval.project()->id());
}

}  // namespace

SqlTimeSheetRepository::SqlTimeSheetRepository(Database& database) : m_database(database)
{
  seed_id_counters();
}

void SqlTimeSheetRepository::seed_id_counters()
{
  // Seeding here rather than in load() keeps a repository that only ever inserts (as in tests)
  // just as safe as one that loaded first.
  const auto highest_id = [this](const QString& table) {
    auto query = m_database.prepare(QStringLiteral("SELECT COALESCE(MAX(id), 0) FROM %1").arg(table));
    m_database.execute(query);
    if (!query.next()) {
      return EntityId::Value{0};
    }
    return static_cast<EntityId::Value>(query.value(0).toLongLong());
  };
  m_next_project_id = highest_id(QStringLiteral("project"));
  m_next_interval_id = highest_id(QStringLiteral("interval"));
  m_next_plan_entry_id = highest_id(QStringLiteral("plan_entry"));
}

EntityId SqlTimeSheetRepository::next_id(EntityId::Value& counter) const noexcept
{
  ++counter;
  return EntityId{counter};
}

void SqlTimeSheetRepository::insert(Project& project)
{
  if (!project.id().is_valid()) {
    project.set_id(next_id(m_next_project_id));
  }
  // Upsert rather than a plain INSERT: after a rolled-back delete the row still exists while the
  // entity is gone from memory, and an undo would otherwise collide on the primary key.
  auto query =
      m_database.prepare(QStringLiteral("INSERT INTO project (id, name, color) VALUES (?, ?, ?) "
                                        "ON CONFLICT (id) DO UPDATE SET name = excluded.name, color = excluded.color"));
  query.addBindValue(::id_to_sql(project.id()));
  query.addBindValue(project.name());
  query.addBindValue(to_sql(project.color()));
  m_database.execute(query);
}

void SqlTimeSheetRepository::update(const Project& project)
{
  auto query = m_database.prepare(QStringLiteral("UPDATE project SET name = ?, color = ? WHERE id = ?"));
  query.addBindValue(project.name());
  query.addBindValue(to_sql(project.color()));
  query.addBindValue(::id_to_sql(project.id()));
  m_database.execute(query);
}

void SqlTimeSheetRepository::remove(const Project& project)
{
  auto query = m_database.prepare(QStringLiteral("DELETE FROM project WHERE id = ?"));
  query.addBindValue(::id_to_sql(project.id()));
  m_database.execute(query);
}

void SqlTimeSheetRepository::insert(Interval& interval)
{
  if (!interval.id().is_valid()) {
    interval.set_id(next_id(m_next_interval_id));
  }
  auto query = m_database.prepare(
      QStringLiteral("INSERT INTO interval (id, project_id, begin_time, end_time) VALUES (?, ?, ?, ?) "
                     "ON CONFLICT (id) DO UPDATE SET project_id = excluded.project_id, "
                     "begin_time = excluded.begin_time, end_time = excluded.end_time"));
  query.addBindValue(::id_to_sql(interval.id()));
  query.addBindValue(::project_reference(interval));
  query.addBindValue(to_sql(interval.begin()));
  query.addBindValue(to_sql(interval.end()));
  m_database.execute(query);
}

void SqlTimeSheetRepository::update(const Interval& interval)
{
  auto query = m_database.prepare(
      QStringLiteral("UPDATE interval SET project_id = ?, begin_time = ?, end_time = ? WHERE id = ?"));
  query.addBindValue(::project_reference(interval));
  query.addBindValue(to_sql(interval.begin()));
  query.addBindValue(to_sql(interval.end()));
  query.addBindValue(::id_to_sql(interval.id()));
  m_database.execute(query);
}

void SqlTimeSheetRepository::remove(const Interval& interval)
{
  auto query = m_database.prepare(QStringLiteral("DELETE FROM interval WHERE id = ?"));
  query.addBindValue(::id_to_sql(interval.id()));
  m_database.execute(query);
}

void SqlTimeSheetRepository::insert(Plan::Entry& entry)
{
  if (!entry.id.is_valid()) {
    entry.id = next_id(m_next_plan_entry_id);
  }
  auto query =
      m_database.prepare(QStringLiteral("INSERT INTO plan_entry (id, period_begin, period_end, period_type, kind) "
                                        "VALUES (?, ?, ?, ?, ?) ON CONFLICT (id) DO UPDATE SET "
                                        "period_begin = excluded.period_begin, period_end = excluded.period_end, "
                                        "period_type = excluded.period_type, kind = excluded.kind"));
  query.addBindValue(::id_to_sql(entry.id));
  query.addBindValue(to_sql(entry.period.begin()));
  query.addBindValue(to_sql(entry.period.end()));
  query.addBindValue(db_name(entry.period.type()));
  query.addBindValue(db_name(entry.kind));
  m_database.execute(query);
}

void SqlTimeSheetRepository::update(const Plan::Entry& entry)
{
  auto query = m_database.prepare(QStringLiteral("UPDATE plan_entry SET period_begin = ?, period_end = ?, "
                                                 "period_type = ?, kind = ? WHERE id = ?"));
  query.addBindValue(to_sql(entry.period.begin()));
  query.addBindValue(to_sql(entry.period.end()));
  query.addBindValue(db_name(entry.period.type()));
  query.addBindValue(db_name(entry.kind));
  query.addBindValue(::id_to_sql(entry.id));
  m_database.execute(query);
}

void SqlTimeSheetRepository::remove(const Plan::Entry& entry)
{
  auto query = m_database.prepare(QStringLiteral("DELETE FROM plan_entry WHERE id = ?"));
  query.addBindValue(::id_to_sql(entry.id));
  m_database.execute(query);
}

void SqlTimeSheetRepository::update_plan_setting(const Plan& plan)
{
  // Upsert, so a repository used without load() (which is what creates the row) still stores the
  // settings instead of silently updating nothing.
  auto query = m_database.prepare(
      QStringLiteral("INSERT INTO plan_setting (id, start_date, overtime_offset_minutes) VALUES (?, ?, ?) "
                     "ON CONFLICT (id) DO UPDATE SET start_date = excluded.start_date, "
                     "overtime_offset_minutes = excluded.overtime_offset_minutes"));
  query.addBindValue(plan_setting_id);
  query.addBindValue(to_sql(plan.start()));
  query.addBindValue(to_sql(plan.overtime_offset()));
  m_database.execute(query);
}

std::unique_ptr<TimeSheet> SqlTimeSheetRepository::load()
{
  // One transaction so the four reads see a consistent snapshot of the database.
  Database::Transaction transaction{m_database};

  auto projects = std::vector<std::unique_ptr<Project>>{};
  auto projects_by_id = std::unordered_map<EntityId::Value, const Project*>{};
  {
    auto query = m_database.prepare(QStringLiteral("SELECT id, name, color FROM project ORDER BY id"));
    m_database.execute(query);
    while (query.next()) {
      auto project = std::make_unique<Project>(query.value(1).toString(), color_from_sql(query.value(2)));
      project->set_id(EntityId{query.value(0).toLongLong()});
      projects_by_id.emplace(project->id().value(), project.get());
      projects.push_back(std::move(project));
    }
  }

  auto intervals = std::deque<std::unique_ptr<Interval>>{};
  {
    auto query =
        m_database.prepare(QStringLiteral("SELECT id, project_id, begin_time, end_time FROM interval ORDER BY id"));
    m_database.execute(query);
    while (query.next()) {
      // Resolving by id replaces the old JSON format's positional reference, which silently
      // mis-associated intervals whenever the project order changed.
      const auto* project = static_cast<const Project*>(nullptr);
      if (!query.value(1).isNull()) {
        const auto project_id = static_cast<EntityId::Value>(query.value(1).toLongLong());
        const auto it = projects_by_id.find(project_id);
        if (it == projects_by_id.end()) {
          throw DatabaseError("Interval {} references project {}, which does not exist.", query.value(0).toLongLong(),
                              project_id);
        }
        project = it->second;
      }
      auto interval = std::make_unique<Interval>(project);
      interval->swap_begin(date_time_from_sql(query.value(2)));
      interval->swap_end(date_time_from_sql(query.value(3)));
      interval->set_id(EntityId{query.value(0).toLongLong()});
      intervals.push_back(std::move(interval));
    }
  }

  auto entries = std::vector<std::unique_ptr<Plan::Entry>>{};
  {
    auto query = m_database.prepare(QStringLiteral(
        "SELECT id, period_begin, period_end, period_type, kind FROM plan_entry ORDER BY period_begin, id"));
    m_database.execute(query);
    while (query.next()) {
      const auto begin = date_from_sql(query.value(1));
      const auto end = date_from_sql(query.value(2));
      const auto type = period_type_from_db_name(query.value(3).toString());
      // A typed period recomputes its end from its begin; only a custom one carries both.
      auto period = (type == Period::Type::Custom) ? Period{begin, end} : Period{begin, type};
      if (period.end() != end) {
        spdlog::warn("Stored period end {} disagrees with the recomputed end {}; using the recomputed one.",
                     end.toString(Qt::ISODate).toStdString(), period.end().toString(Qt::ISODate).toStdString());
      }
      entries.push_back(std::make_unique<Plan::Entry>(period, plan_kind_from_db_name(query.value(4).toString()),
                                                      EntityId{query.value(0).toLongLong()}));
    }
  }

  auto start = Application::current_date_time().date();
  auto overtime_offset = std::chrono::minutes{0};
  {
    auto query =
        m_database.prepare(QStringLiteral("SELECT start_date, overtime_offset_minutes FROM plan_setting WHERE id = ?"));
    query.addBindValue(plan_setting_id);
    m_database.execute(query);
    if (query.next()) {
      start = date_from_sql(query.value(0));
      overtime_offset = minutes_from_sql(query.value(1));
    } else {
      // Fresh database. Pinning the start date now matters: without a stored row, every launch
      // would silently reset it to "today" and the overtime balance would never accumulate.
      auto insert = m_database.prepare(
          QStringLiteral("INSERT INTO plan_setting (id, start_date, overtime_offset_minutes) VALUES (?, ?, ?)"));
      insert.addBindValue(plan_setting_id);
      insert.addBindValue(to_sql(start));
      insert.addBindValue(to_sql(overtime_offset));
      m_database.execute(insert);
    }
  }

  auto time_sheet =
      std::make_unique<TimeSheet>(std::make_unique<ProjectModel>(*this, std::move(projects)),
                                  std::make_unique<IntervalModel>(*this, std::move(intervals)),
                                  std::make_unique<FullTimePlan>(*this, start, overtime_offset, std::move(entries)));
  transaction.commit();
  return time_sheet;
}
