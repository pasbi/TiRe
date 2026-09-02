#include "db/database.h"

#include "db/migrations.h"
#include "exceptions.h"
#include "qtfixture.h"

#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QVariant>
#include <QtGlobal>
#include <gtest/gtest.h>
#if defined(Q_OS_UNIX)
// for ::geteuid()
#  include <unistd.h>
#endif

namespace
{

class DatabaseTest : public QtFixture
{
protected:
  /** @brief Probes for a table portably: sqlite_master is SQLite-only, information_schema is not. */
  [[nodiscard]] static bool has_table(const Database& database, const QString& table)
  {
    try {
      // Selecting a literal rather than a column keeps the probe independent of each table's shape.
      database.execute(QStringLiteral("SELECT 1 FROM %1 WHERE 1 = 0").arg(table));
      return true;
    } catch (const DatabaseError&) {
      return false;
    }
  }

  [[nodiscard]] static int count_rows(const Database& database, const QString& table)
  {
    auto query = database.prepare(QStringLiteral("SELECT COUNT(*) FROM %1").arg(table));
    database.execute(query);
    EXPECT_TRUE(query.next());
    return query.value(0).toInt();
  }
};

}  // namespace

TEST_F(DatabaseTest, MigrateFromScratch)
{
  auto database = Database::open_in_memory();
  EXPECT_EQ(0, database.schema_version());
  database.migrate();
  EXPECT_EQ(latest_schema_version(), database.schema_version());
}

TEST_F(DatabaseTest, TablesExistAfterMigration)
{
  auto database = Database::open_in_memory();
  database.migrate();
  for (const auto* const table : {"project", "interval", "plan_entry", "plan_setting", "schema_version"}) {
    EXPECT_TRUE(has_table(database, QString::fromLatin1(table))) << "missing table: " << table;
  }
}

TEST_F(DatabaseTest, MigrationIsIdempotent)
{
  auto database = Database::open_in_memory();
  database.migrate();
  const auto applied = count_rows(database, QStringLiteral("schema_version"));
  database.migrate();
  EXPECT_EQ(latest_schema_version(), database.schema_version());
  EXPECT_EQ(applied, count_rows(database, QStringLiteral("schema_version")));
}

TEST_F(DatabaseTest, StepwiseMigration)
{
  auto database = Database::open_in_memory();
  database.migrate_to(1);
  EXPECT_EQ(1, database.schema_version());
  database.migrate_to(latest_schema_version());
  EXPECT_EQ(latest_schema_version(), database.schema_version());
}

TEST_F(DatabaseTest, RejectsNewerSchema)
{
  auto database = Database::open_in_memory();
  database.migrate();

  auto query = database.prepare(QStringLiteral("INSERT INTO schema_version (version, applied_at) VALUES (?, ?)"));
  query.addBindValue(latest_schema_version() + 1);
  query.addBindValue(QStringLiteral("2026-01-01T00:00:00"));
  database.execute(query);

  // An older build must refuse a newer schema rather than silently mangling it.
  EXPECT_THROW(database.migrate(), DatabaseError);
}

TEST_F(DatabaseTest, ForeignKeysAreEnforced)
{
  // Canary for `PRAGMA foreign_keys = ON` silently not taking effect: SQLite ignores foreign keys
  // per connection unless it is switched on, and then this insert would wrongly succeed.
  auto database = Database::open_in_memory();
  database.migrate();

  auto query = database.prepare(QStringLiteral("INSERT INTO interval (id, project_id, begin_time) VALUES (?, ?, ?)"));
  query.addBindValue(1);
  query.addBindValue(1234);  // no such project
  query.addBindValue(QStringLiteral("2026-01-01T08:00:00"));
  EXPECT_THROW(database.execute(query), DatabaseError);
}

TEST_F(DatabaseTest, RollbackOnUncommittedTransaction)
{
  auto database = Database::open_in_memory();
  database.migrate();
  {
    Database::Transaction transaction{database};
    auto query = database.prepare(QStringLiteral("INSERT INTO project (id, name, color) VALUES (?, ?, ?)"));
    query.addBindValue(1);
    query.addBindValue(QStringLiteral("abandoned"));
    query.addBindValue(QStringLiteral("#ff000000"));
    database.execute(query);
    // no commit()
  }
  EXPECT_EQ(0, count_rows(database, QStringLiteral("project")));
}

TEST_F(DatabaseTest, NestedTransactionCommitsOnce)
{
  auto database = Database::open_in_memory();
  database.migrate();

  const auto insert_project = [&database](const int id, const QString& name) {
    auto query = database.prepare(QStringLiteral("INSERT INTO project (id, name, color) VALUES (?, ?, ?)"));
    query.addBindValue(id);
    query.addBindValue(name);
    query.addBindValue(QStringLiteral("#ff000000"));
    database.execute(query);
  };

  {
    Database::Transaction outer{database};
    {
      Database::Transaction inner{database};
      insert_project(1, QStringLiteral("inner"));
      inner.commit();
    }
    insert_project(2, QStringLiteral("outer"));
    outer.commit();
  }
  EXPECT_EQ(2, count_rows(database, QStringLiteral("project")));
}

TEST_F(DatabaseTest, AbandonedInnerScopePoisonsTheWholeNest)
{
  auto database = Database::open_in_memory();
  database.migrate();
  {
    Database::Transaction outer{database};
    {
      Database::Transaction inner{database};
      auto query = database.prepare(QStringLiteral("INSERT INTO project (id, name, color) VALUES (?, ?, ?)"));
      query.addBindValue(1);
      query.addBindValue(QStringLiteral("doomed"));
      query.addBindValue(QStringLiteral("#ff000000"));
      database.execute(query);
      // inner is abandoned, so the outer scope must not be able to commit half a compound change.
    }
    EXPECT_THROW(outer.commit(), DatabaseError);
  }
  EXPECT_EQ(0, count_rows(database, QStringLiteral("project")));
}

TEST_F(DatabaseTest, OpeningACorruptFileFails)
{
  // Must be reported, never "repaired" by silently recreating the file -- that would be data loss.
  const QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const auto path = directory.filePath(QStringLiteral("corrupt.db"));
  QFile file{path};
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("this is definitely not a database");
  file.close();

  // The rejection happens at open: SQLite only reads the file header on first access.
  EXPECT_THROW(
      {
        auto database = Database::open_file(std::filesystem::path{path.toStdString()});
        database.migrate();
      },
      DatabaseError);

  // The file must be left exactly as it was, never "repaired" by recreating it.
  QFile unchanged{path};
  ASSERT_TRUE(unchanged.open(QIODevice::ReadOnly));
  EXPECT_EQ("this is definitely not a database", unchanged.readAll());
}

TEST_F(DatabaseTest, OpeningInAnUnwritableDirectoryFails)
{
#if defined(Q_OS_WIN)
  // Not merely a missing ::geteuid(): on Windows QFile::setPermissions maps to the read-only
  // attribute, which does not stop file creation inside a directory, so the setup below cannot
  // produce an unwritable directory at all. Testing this would take a real ACL denying write.
  GTEST_SKIP() << "directory write permissions are not expressible via QFile::setPermissions";
#else
  if (::geteuid() == 0) {
    GTEST_SKIP() << "running as root, which ignores directory permissions";
  }
  const QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const auto sub_directory = QDir{directory.path()}.filePath(QStringLiteral("nowrite"));
  ASSERT_TRUE(QDir{}.mkpath(sub_directory));
  ASSERT_TRUE(QFile::setPermissions(sub_directory, QFileDevice::ReadOwner | QFileDevice::ExeOwner));

  const auto path = std::filesystem::path{QDir{sub_directory}.filePath(QStringLiteral("x.db")).toStdString()};
  EXPECT_THROW(
      {
        auto database = Database::open_file(path);
        database.migrate();
      },
      DatabaseError);

  QFile::setPermissions(sub_directory, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
#endif
}

TEST_F(DatabaseTest, MigrationSurvivesReopen)
{
  const QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const auto path = std::filesystem::path{directory.filePath(QStringLiteral("tire.db")).toStdString()};

  {
    auto database = Database::open_file(path);
    database.migrate();
    EXPECT_EQ(latest_schema_version(), database.schema_version());
  }
  {
    auto database = Database::open_file(path);
    EXPECT_EQ(latest_schema_version(), database.schema_version());
    database.migrate();  // must be a no-op
    EXPECT_EQ(1, count_rows(database, QStringLiteral("schema_version")));
  }
}
