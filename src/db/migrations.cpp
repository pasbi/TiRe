#include "db/migrations.h"

#include "db/database.h"

#include <QString>
#include <array>

namespace
{

void apply_initial_schema(Database& database)
{
  // The DDL below sticks to constructs SQLite and PostgreSQL share, so that switching the Qt
  // driver to QPSQL later stays a connection-string change:
  // - no INTEGER PRIMARY KEY autoincrement (SQLite rowid magic) and no SERIAL (PostgreSQL only);
  //   ids are assigned by the application, see EntityId.
  // - TEXT for dates, times and colors; BIGINT for counts.
  // - named constraints, so a later migration can refer to them.
  database.execute(QStringLiteral(R"(
      CREATE TABLE schema_version (
        version    INTEGER NOT NULL,
        applied_at TEXT    NOT NULL,
        CONSTRAINT pk_schema_version PRIMARY KEY (version)
      ))"));

  database.execute(QStringLiteral(R"(
      CREATE TABLE project (
        id    BIGINT NOT NULL,
        name  TEXT   NOT NULL,
        color TEXT   NOT NULL,
        CONSTRAINT pk_project PRIMARY KEY (id)
      ))"));
  // No UNIQUE on name yet: ProjectModel::add currently accepts duplicates, and a constraint the
  // in-memory model does not enforce would reject a state the model happily produces -- failing
  // only after the mutation already happened. Add it once the model enforces uniqueness.

  database.execute(QStringLiteral(R"(
      CREATE TABLE interval (
        id         BIGINT NOT NULL,
        project_id BIGINT NULL,
        begin_time TEXT   NULL,
        end_time   TEXT   NULL,
        CONSTRAINT pk_interval PRIMARY KEY (id),
        CONSTRAINT fk_interval_project FOREIGN KEY (project_id)
            REFERENCES project (id) ON DELETE RESTRICT
      ))"));
  // end_time IS NULL means the interval is still running.
  //
  // Deliberately no CHECK (end_time >= begin_time). Retiming an interval is two commands --
  // swap_begin then swap_end -- and each writes through, so the row legitimately passes through a
  // state whose begin is after its end. A constraint the in-memory model does not enforce would
  // reject a state the model routinely produces, and the write would fail after the change had
  // already happened in memory. Same reasoning as the missing UNIQUE on project.name above.
  database.execute(QStringLiteral("CREATE INDEX ix_interval_begin_time ON interval (begin_time)"));
  database.execute(QStringLiteral("CREATE INDEX ix_interval_project_id ON interval (project_id)"));

  database.execute(QStringLiteral(R"(
      CREATE TABLE plan_entry (
        id           BIGINT NOT NULL,
        period_begin TEXT   NOT NULL,
        period_end   TEXT   NOT NULL,
        period_type  TEXT   NOT NULL,
        kind         TEXT   NOT NULL,
        CONSTRAINT pk_plan_entry PRIMARY KEY (id),
        CONSTRAINT ck_plan_entry_order CHECK (period_end >= period_begin),
        CONSTRAINT ck_plan_entry_type  CHECK (period_type IN ('YEAR','MONTH','WEEK','DAY','CUSTOM')),
        CONSTRAINT ck_plan_entry_kind  CHECK (kind IN ('NORMAL','SICK','HOLIDAY','HALF_HOLIDAY',
                                                       'VACATION','HALF_VACATION','HALF_VACATION_HALF_HOLIDAY'))
      ))"));
  database.execute(QStringLiteral("CREATE INDEX ix_plan_entry_period_begin ON plan_entry (period_begin)"));

  database.execute(QStringLiteral(R"(
      CREATE TABLE plan_setting (
        id                      BIGINT NOT NULL,
        start_date              TEXT   NOT NULL,
        overtime_offset_minutes BIGINT NOT NULL,
        CONSTRAINT pk_plan_setting PRIMARY KEY (id),
        CONSTRAINT ck_plan_setting_singleton CHECK (id = 1)
      ))"));
}

constexpr auto all_migrations = std::array{
    Migration{.version = 1, .description = "initial schema", .apply = &apply_initial_schema},
};

}  // namespace

std::span<const Migration> migrations()
{
  return all_migrations;
}

int latest_schema_version()
{
  return all_migrations.back().version;
}
