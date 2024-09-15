#pragma once

#include <QDateTime>
#include <filesystem>
#include <memory>
#include <optional>

class QApplication;

class Application
{
public:
  explicit Application(int& argc, char** argv);
  ~Application();

  [[nodiscard]] static QDateTime current_date_time();
  [[nodiscard]] static const std::filesystem::path& timesheet_filename() noexcept;

private:
  std::unique_ptr<QApplication> m_qapp;
  static std::optional<QDateTime> m_current_date_time;
  static std::filesystem::path m_timesheet_filename;
};
