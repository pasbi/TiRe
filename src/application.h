#pragma once

#include <QString>
#include <filesystem>
#include <memory>
#include <optional>

class Database;
class SqlTimeSheetRepository;
class UndoStack;
class QApplication;
class QDateTime;

class Application
{
public:
  explicit Application(int& argc, char** argv);
  ~Application();
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  Application(Application&&) = delete;
  Application& operator=(Application&&) = delete;

  [[nodiscard]] static QDateTime current_date_time();
  [[nodiscard]] static UndoStack& undo_stack() noexcept;
  QApplication& qapp() const noexcept;

  /** @brief Where the timesheet database lives; the --database option overrides the default. */
  [[nodiscard]] static const std::filesystem::path& database_path() noexcept;

  /** @brief Whether database_path() is the standard location rather than an override. */
  [[nodiscard]] static bool is_default_database() noexcept;

  /**
   * @brief A name identifying this database for the single-instance lock.
   * Derived from the path so that "one process per database" holds, rather than
   * "one process for the whole application" -- otherwise a --database override would be ignored
   * whenever the regular instance is running.
   */
  [[nodiscard]] static QString single_instance_name();

  struct DatabaseOpenResult
  {
    bool ok;
    QString message;  ///< Empty when ok; ready to show to the user otherwise.
  };

  /**
   * @brief Opens and migrates the database at database_path().
   * Reports failure rather than throwing, because the caller's only sensible reaction is to show
   * the message and exit.
   */
  [[nodiscard]] DatabaseOpenResult open_database();

  /** @pre open_database() returned ok. */
  [[nodiscard]] static SqlTimeSheetRepository& sql_repository() noexcept;

  /** @brief The open database, or nullptr when none was opened (as in tests and benchmarks). */
  [[nodiscard]] static Database* database() noexcept;

private:
  std::unique_ptr<QApplication> m_qapp;
  // Held by value-owning pointers on the instance rather than as file-scope statics: the
  // connection must be closed while QApplication is still alive, or Qt unloads the SQL driver
  // plugin out from under it.
  std::unique_ptr<Database> m_database;
  std::unique_ptr<SqlTimeSheetRepository> m_repository;
  static std::optional<QDateTime> m_current_date_time;
  static std::filesystem::path m_database_path;
  static bool m_is_default_database;
  static std::unique_ptr<UndoStack> m_undo_stack;
  static Database* m_database_instance;
  static SqlTimeSheetRepository* m_repository_instance;
};
