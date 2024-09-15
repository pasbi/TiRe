#include "application.h"
#include "fmt.h"
#include <QApplication>
#include <QCommandLineParser>

std::optional<QDateTime> Application::m_current_date_time = std::nullopt;
std::filesystem::path Application::m_timesheet_filename = {};

namespace
{

constexpr auto timesheet_filename_option_name = "timesheet-filename";
constexpr auto current_date_time_option_name = "current-date-time";

[[nodiscard]] auto command_line_args()
{
  auto clp = std::make_unique<QCommandLineParser>();
  clp->addPositionalArgument(timesheet_filename_option_name, "Path to the time sheet (JSON).", "FILENAME");
  clp->addOption(QCommandLineOption{
      current_date_time_option_name,
      "Fix the current date time to this value (ISO format). Useful for reproducible debugging and testing.",
      "CURRENT_DATE_TIME"});
  clp->process(*QApplication::instance());
  return clp;
}

}  // namespace

Application::Application(int& argc, char** argv) : m_qapp(std::make_unique<QApplication>(argc, argv))
{
  const auto args = command_line_args();
  if (const auto v = args->value(current_date_time_option_name); !v.isEmpty()) {
    m_current_date_time = QDateTime::fromString(v, Qt::ISODate);
    if (!m_current_date_time->isValid()) {
      fmt::println("Value '{}' is not a valid ISO date time format.", v);
      QApplication::exit(1);
    } else {
      fmt::println("Simulating today = {}", *m_current_date_time);
    }
  }
  if (const auto filenames = args->positionalArguments(); !filenames.empty()) {
    m_timesheet_filename = static_cast<std::filesystem::path>(filenames.front().toStdString());
  }
}

Application::~Application() = default;

QDateTime Application::current_date_time()
{
  if (m_current_date_time.has_value()) {
    return *m_current_date_time;
  }
  return QDateTime::currentDateTime();
}

const std::filesystem::path& Application::timesheet_filename() noexcept
{
  return m_timesheet_filename;
}
