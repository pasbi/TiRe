#include "application.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char** argv)
{
  Application app(argc, argv);

  MainWindow w;
  if (const auto& filename = Application::timesheet_filename(); !filename.empty()) {
    w.load(filename);
  }
  w.show();
  QApplication::exec();
}
