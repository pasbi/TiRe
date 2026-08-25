#include "application.h"
#include "commands/undostack.h"
#include "db/database.h"
#include "db/sqltimesheetrepository.h"
#include "exceptions.h"
#include "fmt.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

std::optional<QDateTime> Application::m_current_date_time = std::nullopt;
std::filesystem::path Application::m_database_path = {};
bool Application::m_is_default_database = true;
std::unique_ptr<UndoStack> Application::m_undo_stack = std::make_unique<UndoStack>();
Database* Application::m_database_instance = nullptr;
SqlTimeSheetRepository* Application::m_repository_instance = nullptr;

namespace
{

constexpr auto database_option_name = "database";
constexpr auto current_date_time_option_name = "current-date-time";
constexpr auto database_file_name = "tire.db";

[[nodiscard]] auto command_line_args()
{
  auto clp = std::make_unique<QCommandLineParser>();
  clp->addOption(QCommandLineOption{
      database_option_name,
      "Use this database instead of the one in the application data directory. Useful for testing.", "PATH"});
  clp->addOption(QCommandLineOption{
      current_date_time_option_name,
      "Fix the current date time to this value (ISO format). Useful for reproducible debugging and testing.",
      "CURRENT_DATE_TIME"});
  clp->addHelpOption();
  clp->addVersionOption();
  clp->process(*QApplication::instance());
  return clp;
}

[[nodiscard]] std::filesystem::path default_database_path()
{
  // Requires setApplicationName/setOrganizationName to have run already.
  const auto directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  return static_cast<std::filesystem::path>(QDir{directory}.filePath(database_file_name).toStdString());
}

}  // namespace

Application::Application(int& argc, char** argv) : m_qapp(std::make_unique<QApplication>(argc, argv))
{
  // Must precede any QStandardPaths query, and fixes the empty caption every message box in the
  // application would otherwise show.
  //
  // The organization name is deliberately left unset: QStandardPaths::AppDataLocation appends
  // both organization and application, so setting both would nest the database in
  // ~/.local/share/tire/tire/. Nothing here uses QSettings, so the organization buys nothing.
  QCoreApplication::setApplicationName(QStringLiteral("tire"));
  QGuiApplication::setApplicationDisplayName(QObject::tr("TiRe"));

  const auto args = command_line_args();
  if (const auto v = args->value(current_date_time_option_name); !v.isEmpty()) {
    m_current_date_time = QDateTime::fromString(v, Qt::ISODate);
    if (!m_current_date_time->isValid()) {
      fmt::println("Value '{}' is not a valid ISO date time format.", v);
      m_current_date_time = std::nullopt;
    } else {
      fmt::println("Simulating today = {}", *m_current_date_time);
    }
  }

  if (const auto v = args->value(database_option_name); !v.isEmpty()) {
    m_database_path = static_cast<std::filesystem::path>(v.toStdString());
    m_is_default_database = false;
  } else {
    m_database_path = ::default_database_path();
    m_is_default_database = true;
  }
}

Application::~Application()
{
  // Order matters: the repository refers to the database, and the database must be gone before
  // m_qapp is destroyed so that the SQL driver plugin is still loaded when the connection closes.
  m_repository_instance = nullptr;
  m_database_instance = nullptr;
  m_repository.reset();
  m_database.reset();
}

QDateTime Application::current_date_time()
{
  if (m_current_date_time.has_value()) {
    return *m_current_date_time;
  }
  return QDateTime::currentDateTime();
}

const std::filesystem::path& Application::database_path() noexcept
{
  return m_database_path;
}

bool Application::is_default_database() noexcept
{
  return m_is_default_database;
}

QString Application::single_instance_name()
{
  // Hash rather than embed the path: the socket name has a length limit that would truncate a
  // long path, and truncation could alias two different databases onto one lock.
  const auto canonical = QFileInfo{QString::fromStdString(m_database_path.string())}.absoluteFilePath();
  const auto digest = QCryptographicHash::hash(canonical.toUtf8(), QCryptographicHash::Sha1);
  return QStringLiteral("tire-%1").arg(QString::fromLatin1(digest.toHex().left(8)));
}

Application::DatabaseOpenResult Application::open_database()
{
  const auto path = QString::fromStdString(m_database_path.string());
  const auto directory = QFileInfo{path}.absolutePath();
  if (!QDir{}.mkpath(directory)) {
    return {.ok = false, .message = QObject::tr("Cannot create the directory '%1' for the database.").arg(directory)};
  }

  try {
    m_database = std::make_unique<Database>(Database::open_file(m_database_path));
    m_database->migrate();
    m_repository = std::make_unique<SqlTimeSheetRepository>(*m_database);
  } catch (const DatabaseError& e) {
    m_repository.reset();
    m_database.reset();
    return {.ok = false, .message = QString::fromStdString(e.what())};
  }

  m_database_instance = m_database.get();
  m_repository_instance = m_repository.get();
  return {.ok = true, .message = {}};
}

Database* Application::database() noexcept
{
  return m_database_instance;
}

SqlTimeSheetRepository& Application::sql_repository() noexcept
{
  return *m_repository_instance;
}

UndoStack& Application::undo_stack() noexcept
{
  return *m_undo_stack;
}

QApplication& Application::qapp() const noexcept
{
  return *m_qapp;
}
