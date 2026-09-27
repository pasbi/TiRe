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

void split_plan_entry_kinds_into_kind_and_share(Database& database)
{
  // A kind used to spell out how much of a day it claimed (HALF_VACATION, and even
  // HALF_VACATION_HALF_HOLIDAY for the one combination anybody needed), which made every new
  // combination a new enumerator. Kind now names the leave only and share_permille says how much
  // of the day it takes, so two kinds can share a day by overlapping -- which is what the old
  // combined enumerator becomes here: two entries of half a day each.
  //
  // SQLite cannot alter a CHECK constraint in place, so the table is rebuilt around it: create,
  // copy, drop, rename. PostgreSQL would instead DROP and ADD the one constraint, and would in
  // fact choke on this version, because pk_plan_entry would exist on both tables at once --
  // worth revisiting if the QPSQL switch the initial schema was written for happens.
  database.execute(QStringLiteral(R"(
      CREATE TABLE plan_entry_migrated (
        id             BIGINT NOT NULL,
        period_begin   TEXT   NOT NULL,
        period_end     TEXT   NOT NULL,
        period_type    TEXT   NOT NULL,
        kind           TEXT   NOT NULL,
        share_permille BIGINT NOT NULL,
        CONSTRAINT pk_plan_entry PRIMARY KEY (id),
        CONSTRAINT ck_plan_entry_order CHECK (period_end >= period_begin),
        CONSTRAINT ck_plan_entry_type  CHECK (period_type IN ('YEAR','MONTH','WEEK','DAY','CUSTOM')),
        CONSTRAINT ck_plan_entry_kind  CHECK (kind IN ('SICK','CHILD_SICK','HOLIDAY','VACATION')),
        CONSTRAINT ck_plan_entry_share CHECK (share_permille BETWEEN 1 AND 1000)
      ))"));
  // NORMAL rows are dropped rather than translated: an ordinary working day is now the absence
  // of an entry, so keeping them would book leave of no kind at all.
  database.execute(QStringLiteral(R"(
      INSERT INTO plan_entry_migrated (id, period_begin, period_end, period_type, kind, share_permille)
      SELECT id, period_begin, period_end, period_type,
             CASE kind
               WHEN 'SICK'                       THEN 'SICK'
               WHEN 'HOLIDAY'                    THEN 'HOLIDAY'
               WHEN 'HALF_HOLIDAY'               THEN 'HOLIDAY'
               WHEN 'VACATION'                   THEN 'VACATION'
               WHEN 'HALF_VACATION'              THEN 'VACATION'
               WHEN 'HALF_VACATION_HALF_HOLIDAY' THEN 'VACATION'
               ELSE kind
             END,
             CASE WHEN kind IN ('HALF_HOLIDAY','HALF_VACATION','HALF_VACATION_HALF_HOLIDAY') THEN 500 ELSE 1000 END
      FROM plan_entry WHERE kind <> 'NORMAL')"));
  // An unmapped kind falls through as itself and trips ck_plan_entry_kind, so an upgrade of a
  // database this build does not understand fails loudly instead of quietly becoming vacation.
  // The holiday half of each former HALF_VACATION_HALF_HOLIDAY, as an entry of its own. Ids are
  // assigned by the application (see EntityId), so they simply continue past the highest one.
  database.execute(QStringLiteral(R"(
      INSERT INTO plan_entry_migrated (id, period_begin, period_end, period_type, kind, share_permille)
      SELECT (SELECT MAX(id) FROM plan_entry) + ROW_NUMBER() OVER (ORDER BY id),
             period_begin, period_end, period_type, 'HOLIDAY', 500
      FROM plan_entry WHERE kind = 'HALF_VACATION_HALF_HOLIDAY')"));
  database.execute(QStringLiteral("DROP TABLE plan_entry"));
  database.execute(QStringLiteral("ALTER TABLE plan_entry_migrated RENAME TO plan_entry"));
  // Dropping the old table took its index with it.
  database.execute(QStringLiteral("CREATE INDEX ix_plan_entry_period_begin ON plan_entry (period_begin)"));
}

constexpr auto all_migrations = std::array{
    Migration{.version = 1, .description = "initial schema", .apply = &apply_initial_schema},
    Migration{.version = 2,
              .description = "leave kind and share instead of combined kinds",
              .apply = &split_plan_entry_kinds_into_kind_and_share},
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
