#include "mainwindow.h"
#include <QApplication>

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  MainWindow w;
  if (const auto args = QApplication::arguments(); args.size() > 1) {
    w.load(static_cast<std::filesystem::path>(args.at(1).toStdString()));
  }
  w.show();
  QApplication::exec();
}
