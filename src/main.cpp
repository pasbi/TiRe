#include "application.h"
#include "db/sqltimesheetrepository.h"
#include "exceptions.h"
#include "kdsingleapplication/kdsingleapplication.h"
#include "mainwindow.h"
#include "timesheet.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char** argv)
{
  Application app(argc, argv);

  // The single-instance lock is named after the database, so the invariant is one process per
  // database rather than one process overall -- otherwise a --database override would be
  // swallowed whenever the regular instance happens to be running.
  KDSingleApplication kdsa{Application::single_instance_name()};
  if (!kdsa.isPrimaryInstance()) {
    kdsa.sendMessage({});
    return 0;
  }

  // Only the primary instance touches the database, hence the check above this point.
  if (const auto result = app.open_database(); !result.ok) {
    QMessageBox::critical(nullptr, QApplication::applicationDisplayName(), result.message);
    return 1;
  }

  auto time_sheet = std::unique_ptr<TimeSheet>{};
  try {
    time_sheet = Application::sql_repository().load();
  } catch (const DatabaseError& e) {
    QMessageBox::critical(
        nullptr, QApplication::applicationDisplayName(),
        QObject::tr("Cannot read the database '%1': %2")
            .arg(QString::fromStdString(Application::database_path().string()), QString::fromStdString(e.what())));
    return 1;
  }

  // The window is constructed with its data, so the views are never wired to a placeholder.
  MainWindow w{std::move(time_sheet)};
  QObject::connect(&kdsa, &KDSingleApplication::messageReceived, &app.qapp(), [&w](const QByteArray&) {
    w.setWindowState((w.windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    w.raise();  // for MacOS
    w.activateWindow();  // for Windows
  });
  w.show();

  return QApplication::exec();
}
