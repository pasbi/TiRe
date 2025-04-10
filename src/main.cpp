#include "application.h"
#include "kdsingleapplication/kdsingleapplication.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char** argv)
{
  Application app(argc, argv);
  KDSingleApplication kdsa;
  MainWindow w;
  if (kdsa.isPrimaryInstance()) {
    QObject::connect(&kdsa, &KDSingleApplication::messageReceived, &app.qapp(), [&w](const QByteArray&) {
      w.setWindowState((w.windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
      w.raise();  // for MacOS
      w.activateWindow();  // for Windows
    });
    if (const auto& filename = Application::timesheet_filename(); !filename.empty()) {
      w.load(filename);
    }
    w.show();
  } else {
    kdsa.sendMessage({});
    return 0;
  }

  QApplication::exec();
}
