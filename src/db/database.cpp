#include "db/database.h"

#include "db/migrations.h"
#include "exceptions.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QUuid>
#include <QVariant>
#include <spdlog/spdlog.h>

namespace
{

constexpr auto sqlite_driver = "QSQLITE";
constexpr auto in_memory_path = ":memory:";

[[nodiscard]] QString make_connection_name()
{
  return QStringLiteral("tire-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

[[nodiscard]] std::string error_text(const QSqlError& error)
{
  return error.text().trimmed().toStdString();
}

}  // namespace

Database::Database(QString connection_name, const Dialect dialect)
  : m_connection_name(std::move(connection_name)), m_dialect(dialect), m_thread(QThread::currentThread())
{
}

// Moving is only meaningful for a Database with no open transaction: a live Transaction holds a
// reference to its Database, so moving out from under one would leave it decrementing a counter
// that no longer belongs to the connection. Both operations therefore leave the source fully
// neutralized, so its destructor does nothing.
Database::Database(Database&& other) noexcept
  : m_connection_name(std::move(other.m_connection_name))
  , m_dialect(other.m_dialect)
  , m_thread(other.m_thread)
  , m_transaction_depth(other.m_transaction_depth)
  , m_transaction_poisoned(other.m_transaction_poisoned)
{
  Q_ASSERT(m_transaction_depth == 0);
  other.m_connection_name.clear();
  other.m_transaction_depth = 0;
  other.m_transaction_poisoned = false;
  other.m_thread = nullptr;
}

Database& Database::operator=(Database&& other) noexcept
{
  if (this != &other) {
    Q_ASSERT(m_transaction_depth == 0 && other.m_transaction_depth == 0);
    if (!m_connection_name.isEmpty()) {
      QSqlDatabase::removeDatabase(m_connection_name);
    }
    m_connection_name = std::move(other.m_connection_name);
    m_dialect = other.m_dialect;
    m_thread = other.m_thread;
    m_transaction_depth = other.m_transaction_depth;
    m_transaction_poisoned = other.m_transaction_poisoned;
    other.m_connection_name.clear();
    other.m_transaction_depth = 0;
    other.m_transaction_poisoned = false;
    other.m_thread = nullptr;
  }
  return *this;
}

Database::~Database()
{
  if (m_connection_name.isEmpty()) {
    return;
  }
  {
    // The handle must go out of scope before removeDatabase, or Qt warns that the connection is
    // still in use and keeps it alive.
    auto database = QSqlDatabase::database(m_connection_name, false);
    if (database.isOpen() && m_transaction_depth > 0) {
      spdlog::error("Closing the database with an unfinished transaction; rolling it back.");
      database.rollback();
    }
  }
  QSqlDatabase::removeDatabase(m_connection_name);
}

Database Database::open_file(const std::filesystem::path& path)
{
  if (!QSqlDatabase::isDriverAvailable(sqlite_driver)) {
    throw DatabaseError("The {} database driver is not available. The Qt SQL drivers are plugins; "
                        "this build of Qt appears to ship without the SQLite one.",
                        sqlite_driver);
  }

  auto connection_name = make_connection_name();
  {
    auto database = QSqlDatabase::addDatabase(sqlite_driver, connection_name);
    database.setDatabaseName(QString::fromStdString(path.string()));
    if (!database.open()) {
      const auto message = error_text(database.lastError());
      database = {};
      QSqlDatabase::removeDatabase(connection_name);
      throw DatabaseError("Cannot open the database '{}': {}", path.string(), message);
    }
  }

  Database result{std::move(connection_name), Dialect::SQLITE};
  try {
    // SQLite ignores foreign keys unless they are switched on per connection; PostgreSQL always
    // enforces them. WAL plus synchronous=NORMAL is the right trade for a desktop app doing one
    // small transaction per user action.
    result.execute(QStringLiteral("PRAGMA foreign_keys = ON"));
    if (path != in_memory_path) {
      result.execute(QStringLiteral("PRAGMA journal_mode = WAL"));
      result.execute(QStringLiteral("PRAGMA synchronous = NORMAL"));
    }
    result.execute(QStringLiteral("PRAGMA busy_timeout = 5000"));
  } catch (const DatabaseError& e) {
    // SQLite defers reading the file header until the first real access, so this is where a file
    // that is not a database, or is locked by another process, actually surfaces. Report it in
    // terms of the file rather than of the pragma that happened to trip over it.
    throw DatabaseError("Cannot use '{}' as a database. It may be corrupt, or in use by another "
                        "program. Details: {}",
                        path.string(), e.what());
  }
  return result;
}

Database Database::open_in_memory()
{
  return open_file(in_memory_path);
}

Database::Dialect Database::dialect() const noexcept
{
  return m_dialect;
}

bool Database::in_transaction() const noexcept
{
  return m_transaction_depth > 0;
}

void Database::verify_thread() const
{
  if (QThread::currentThread() != m_thread) {
    throw DatabaseError("The database was used from a thread other than the one that opened it. "
                        "A QSqlDatabase connection may only be used from its creating thread.");
  }
}

QSqlQuery Database::prepare(const QString& statement) const
{
  verify_thread();
  QSqlQuery query{QSqlDatabase::database(m_connection_name, false)};
  if (!query.prepare(statement)) {
    throw DatabaseError("Cannot prepare '{}': {}", statement.toStdString(), error_text(query.lastError()));
  }
  return query;
}

void Database::execute(QSqlQuery& query) const
{
  verify_thread();
  if (!query.exec()) {
    throw DatabaseError("Cannot execute '{}': {}", query.lastQuery().toStdString(), error_text(query.lastError()));
  }
}

void Database::execute(const QString& statement) const
{
  auto query = prepare(statement);
  execute(query);
}

int Database::schema_version() const
{
  // Probing with a plain SELECT keeps this portable: sqlite_master is SQLite-only and
  // information_schema is PostgreSQL-only. A missing table simply means "not migrated yet".
  try {
    auto query = prepare(QStringLiteral("SELECT MAX(version) FROM schema_version"));
    execute(query);
    if (!query.next() || query.value(0).isNull()) {
      return 0;
    }
    return query.value(0).toInt();
  } catch (const DatabaseError&) {
    return 0;
  }
}

void Database::migrate()
{
  migrate_to(latest_schema_version());
}

void Database::migrate_to(const int target_version)
{
  const auto current_version = schema_version();
  if (current_version > latest_schema_version()) {
    throw DatabaseError("The database has schema version {}, but this build only understands up to {}. "
                        "Refusing to open it, because an older build could corrupt a newer schema.",
                        current_version, latest_schema_version());
  }

  for (const auto& migration : migrations()) {
    if (migration.version <= current_version || migration.version > target_version) {
      continue;
    }
    spdlog::info("Applying schema migration {}: {}", migration.version, migration.description);
    // Each migration commits on its own, so a failure leaves the database at the previous
    // version rather than half-migrated. Both SQLite and PostgreSQL have transactional DDL.
    Transaction transaction{*this};
    migration.apply(*this);
    auto query = prepare(QStringLiteral("INSERT INTO schema_version (version, applied_at) VALUES (?, ?)"));
    query.addBindValue(migration.version);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    execute(query);
    transaction.commit();
  }
}

Database::Transaction::Transaction(Database& database) : m_database(database)
{
  m_database.verify_thread();
  if (m_database.m_transaction_depth == 0) {
    m_database.m_transaction_poisoned = false;
    if (!QSqlDatabase::database(m_database.m_connection_name, false).transaction()) {
      throw DatabaseError("Cannot begin a transaction.");
    }
  }
  ++m_database.m_transaction_depth;
}

void Database::Transaction::commit()
{
  if (m_committed) {
    return;
  }
  if (m_database.m_transaction_poisoned) {
    throw DatabaseError("Cannot commit: an enclosed transaction scope was abandoned.");
  }
  // The COMMIT happens here rather than in the destructor so that a failure is throwable and the
  // caller can react. Inner scopes only record their success; the outermost one does the work.
  if (m_database.m_transaction_depth == 1) {
    auto database = QSqlDatabase::database(m_database.m_connection_name, false);
    if (!database.commit()) {
      throw DatabaseError("Cannot commit the transaction: {}", error_text(database.lastError()));
    }
  }
  m_committed = true;
}

Database::Transaction::~Transaction()
{
  if (!m_committed) {
    m_database.m_transaction_poisoned = true;
  }
  --m_database.m_transaction_depth;
  if (m_database.m_transaction_depth > 0) {
    return;
  }

  if (m_database.m_transaction_poisoned) {
    // A destructor is implicitly noexcept, and both the rollback reporting and spdlog allocate,
    // so anything thrown here would terminate the process.
    try {
      auto database = QSqlDatabase::database(m_database.m_connection_name, false);
      if (!database.rollback()) {
        spdlog::error("Failed to roll back the transaction: {}", error_text(database.lastError()));
      }
    } catch (...) {  // NOLINT(bugprone-empty-catch)
    }
  }
  m_database.m_transaction_poisoned = false;
}
