#include "application.h"
#include "db/sqltimesheetrepository.h"
#include "exceptions.h"
#include "kdsingleapplication/kdsingleapplication.h"
#include "mainwindow.h"
#include "timesheet.h"
#include <QApplication>
#include <QMessageBox>
#include <QWidget>

namespace
{

/**
 * @brief The activation token, or empty when the launcher passed none.
 * Call this before constructing QApplication: Qt clears the variables once it has spent the token
 * on a window of its own.
 */
[[nodiscard]] QByteArray launcher_activation_token()
{
  // Wayland compositors and X11 startup notification use different names for the same handover.
  auto token = qgetenv("XDG_ACTIVATION_TOKEN");
  if (token.isEmpty()) {
    token = qgetenv("DESKTOP_STARTUP_ID");
  }
  return token;
}

/**
 * @brief Brings @p window to the front on behalf of the launcher identified by @p activation_token.
 * A compositor only lets a window take focus when it can show that a user action asked for it. The
 * secondary instance is that user action, so it hands its token over and the primary instance
 * presents it here -- without it, raising is downgraded to a taskbar highlight.
 */
void raise_to_foreground(QWidget& window, const QByteArray& activation_token)
{
  if (!activation_token.isEmpty()) {
    qputenv("XDG_ACTIVATION_TOKEN", activation_token);
    qputenv("DESKTOP_STARTUP_ID", activation_token);
  }
  window.setWindowState((window.windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
  window.show();  // a no-op while visible, but the window may have been hidden
  window.raise();  // for MacOS
  window.activateWindow();  // for Windows and Wayland
}

}  // namespace

int main(int argc, char** argv)
{
  const auto activation_token = ::launcher_activation_token();

  Application app(argc, argv);

  // The single-instance lock is named after the database, so the invariant is one process per
  // database rather than one process overall -- otherwise a --database override would be
  // swallowed whenever the regular instance happens to be running.
  KDSingleApplication kdsa{Application::single_instance_name()};
  if (!kdsa.isPrimaryInstance()) {
    // The token is the only payload: it lets the primary instance raise itself in our stead.
    kdsa.sendMessage(activation_token);
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
  QObject::connect(&kdsa, &KDSingleApplication::messageReceived, &app.qapp(),
                   [&w](const QByteArray& token) { ::raise_to_foreground(w, token); });
  w.show();

  return QApplication::exec();
}
