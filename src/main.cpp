#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>

namespace
{

constexpr auto timesheet_filename_option_name = "timesheet-filename";

[[nodiscard]] auto command_line_args()
{
  auto clp = std::make_unique<QCommandLineParser>();
  clp->addPositionalArgument(timesheet_filename_option_name, "Path to the time sheet (JSON).", "FILENAME");
  clp->process(*QApplication::instance());
  return clp;
}

}  // namespace

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  MainWindow w;
  const auto clp = command_line_args();
  if (const auto& args = clp->positionalArguments(); !args.empty()) {
    w.load(static_cast<std::filesystem::path>(args.front().toStdString()));
  }
  w.show();
  QApplication::exec();
}
