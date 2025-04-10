#pragma once

#include <filesystem>
#include <memory>
#include <optional>

class UndoStack;
class QApplication;
class QDateTime;

class Application
{
public:
  explicit Application(int& argc, char** argv);
  ~Application();

  [[nodiscard]] static QDateTime current_date_time();
  [[nodiscard]] static const std::filesystem::path& timesheet_filename() noexcept;
  [[nodiscard]] static UndoStack& undo_stack() noexcept;
  QApplication& qapp() const noexcept;

private:
  std::unique_ptr<QApplication> m_qapp;
  static std::optional<QDateTime> m_current_date_time;
  static std::filesystem::path m_timesheet_filename;
  static std::unique_ptr<UndoStack> m_undo_stack;
};
