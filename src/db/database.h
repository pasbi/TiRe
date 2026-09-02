#pragma once

#include <QString>
#include <filesystem>

class QSqlQuery;
class QThread;

/**
 * @class Database database.h "db/database.h"
 * @brief Owns one SQL connection and the schema migrations applied to it.
 *
 * The class deliberately stores only the connection *name*, never a QSqlDatabase member.
 * QSqlDatabase is a reference-counted handle, and QSqlDatabase::removeDatabase() warns (and
 * leaks the connection) if any copy of the handle is still alive when it runs. Keeping only the
 * name means every operation takes a short-lived local handle and the destructor can clean up.
 *
 * The connection name is unique per instance, so several independent in-memory databases can
 * coexist in one process -- which is what lets each test have its own.
 */
class Database
{
public:
  enum class Dialect { SQLITE, POSTGRES };

  /**
   * @brief Opens (creating if absent) the database file at @p path.
   * @throws DatabaseError if the driver is unavailable or the file cannot be opened.
   */
  [[nodiscard]] static Database open_file(const std::filesystem::path& path);

  /**
   * @brief Opens a private, empty, in-memory database.
   * Each call yields a *distinct* database -- an in-memory connection is never shared.
   */
  [[nodiscard]] static Database open_in_memory();

  ~Database();
  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;
  Database(Database&& other) noexcept;
  Database& operator=(Database&& other) noexcept;

  [[nodiscard]] Dialect dialect() const noexcept;

  /**
   * @brief Applies every migration newer than the current schema version.
   * @throws DatabaseError if the stored version is newer than this build understands.
   */
  void migrate();

  /** @brief Applies migrations up to (and including) @p target_version. Exposed for tests. */
  void migrate_to(int target_version);

  /** @brief The currently stored schema version, or 0 if the database is empty. */
  [[nodiscard]] int schema_version() const;

  /** @brief Whether a transaction scope is currently open. */
  [[nodiscard]] bool in_transaction() const noexcept;

  /** @throws DatabaseError if @p statement cannot be prepared. */
  [[nodiscard]] QSqlQuery prepare(const QString& statement) const;

  /** @throws DatabaseError if the query fails. */
  void execute(QSqlQuery& query) const;

  /** @brief Prepares and runs a statement taking no parameters. */
  void execute(const QString& statement) const;

  /**
   * @class Transaction database.h "db/database.h"
   * @brief RAII transaction scope, safe to nest.
   *
   * Only the outermost scope issues BEGIN and COMMIT; inner scopes just adjust a depth counter.
   * That lets a model mutator open a transaction unconditionally without knowing whether an undo
   * macro already opened one around it.
   *
   * If any scope is destroyed without commit() the whole nest is poisoned and the outermost one
   * rolls back, so a failure deep inside a compound command cannot leave half of it committed.
   */
  class Transaction
  {
  public:
    explicit Transaction(Database& database);
    ~Transaction();
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&&) = delete;
    Transaction& operator=(Transaction&&) = delete;

    void commit();

  private:
    Database& m_database;
    bool m_committed = false;
  };

private:
  explicit Database(QString connection_name, Dialect dialect);

  /** @brief Throws DatabaseError if this is used from a thread other than the opening one. */
  void verify_thread() const;

  QString m_connection_name;
  Dialect m_dialect = Dialect::SQLITE;
  const QThread* m_thread = nullptr;
  int m_transaction_depth = 0;
  bool m_transaction_poisoned = false;
};
