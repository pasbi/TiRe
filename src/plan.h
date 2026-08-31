#pragma once

#include "application.h"
#include "db/entityid.h"
#include "fmt.h"
#include "period.h"

#include <QAbstractTableModel>
#include <QDate>
#include <chrono>

class AbstractTimeSheetRepository;
class IntervalModel;
class QDate;

class Plan : public QAbstractTableModel
{
  Q_OBJECT
public:
  static constexpr auto period_column = 0;
  static constexpr auto kind_column = 1;

  enum class Kind { Normal, Sick, Holiday, HalfHoliday, Vacation, HalfVacation, HalfVacationHalfHoliday };

  struct Entry
  {
    Period period;
    Kind kind = Kind::Normal;
    /**
     * @brief Identity of this entry's row, invalid until it has been persisted.
     * Deliberately the last member and deliberately public: Entry is an aggregate, initialized
     * both positionally and with designated initializers, and adding a base class or a private
     * member would break those call sites.
     */
    EntityId id;
  };

  /** @brief Creates a plan that does not persist anything. */
  explicit Plan();
  explicit Plan(AbstractTimeSheetRepository& repository);
  /** @brief Adopts already-stored entries without writing them back. */
  explicit Plan(AbstractTimeSheetRepository& repository, const QDate& start, std::chrono::minutes overtime_offset,
                std::vector<std::unique_ptr<Entry>> entries);
  [[nodiscard]] std::chrono::minutes planned_working_time(const Period& period,
                                                          const IntervalModel& interval_model) const;
  [[nodiscard]] const std::chrono::minutes& overtime_offset() const noexcept;
  [[nodiscard]] const QDate& start() const noexcept;

  /**
   * @name Plan settings
   * The date overtime accounting starts from, and a manual correction added to the balance.
   * Both swap in the new value, persist, and return the previous one.
   * @{
   */
  QDate swap_start(QDate start);
  std::chrono::minutes swap_overtime_offset(std::chrono::minutes overtime_offset);
  /** @} */

  [[nodiscard]] Kind find_kind(const QDate& date) const;

  /**
   * @brief return a period which doesn't overlap with any period in this plan.
   *  Return the period of today or of the date one day after the end of the last period in this plan.
   */
  [[nodiscard]] Period default_period() const noexcept;

  [[nodiscard]] int columnCount(const QModelIndex& parent) const override;
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;

  bool add(std::unique_ptr<Entry> entry);
  std::unique_ptr<Entry> extract(const Entry& entry);

  [[nodiscard]] const Entry& entry(int row) const noexcept;

  /**
   * @name Mutators
   * Each swaps in a new value, persists the entry and returns the previous value, so it can serve
   * as its own inverse in a ModifyCommand.
   *
   * They identify the entry by reference rather than by row on purpose: changing a period
   * re-sorts the entries, so a row index captured before the change refers to a different entry
   * afterwards -- and an undo keyed on it would silently rewrite the wrong one.
   * @{
   */
  Kind swap_kind(const Entry& entry, Kind kind);
  /** @throws RuntimeError if @p period would overlap another entry. See can_set_period(). */
  Period swap_period(const Entry& entry, Period period);
  /** @} */

  /**
   * @brief Whether swap_period() would succeed.
   * Lets a caller check before pushing an undo command, rather than having one throw from redo().
   */
  [[nodiscard]] bool can_set_period(const Entry& entry, const Period& period) const;

  [[nodiscard]] std::chrono::minutes sick_time(const Period& period) const;
  [[nodiscard]] std::chrono::minutes holiday_time(const Period& period) const;
  [[nodiscard]] std::chrono::minutes vacation_time(const Period& period) const;
  [[nodiscard]] std::vector<Kind> kinds_in(const Period& period) const;

Q_SIGNALS:
  void plan_changed();

protected:
  [[nodiscard]] virtual std::chrono::minutes planned_normal_working_time(const QDate& date) const noexcept = 0;

private:
  AbstractTimeSheetRepository& m_repository;
  QDate m_start = Application::current_date_time().date();
  std::chrono::minutes m_overtime_offset{0};
  std::vector<std::unique_ptr<Entry>> m_periods;
  void data_changed(int row, int column);
  /** @throws RuntimeError if @p entry does not belong to this plan. */
  [[nodiscard]] Entry& find_entry(const Entry& entry);
  [[nodiscard]] int row_of(const Entry& entry) const;
  template<typename LeaveFactors> [[nodiscard]] std::chrono::minutes count(const Period& period) const;
  [[nodiscard]] std::chrono::minutes planned_normal_working_time(const Period& period) const noexcept;
  [[nodiscard]] std::chrono::minutes planned_working_time(const QDate& date, Kind kind,
                                                          const IntervalModel& interval_model) const noexcept;
  /**
   * @brief Sorts the periods.
   * The periods are supposed to be sorted at any time, i.e., this function must only be called if the ordering has
   * been destroyed (e.g., after changing the beginning or end of a period in m_periods).
   * Sorting may fail (i.e., if periods overlap).
   * To check if sorting succeeded, use ::is_sorted.
   */
  void sort() noexcept;

  /**
   * @brief checks if the periods are sorted.
   */
  [[nodiscard]] bool is_sorted() const noexcept;
};

class FullTimePlan : public Plan
{
public:
  using Plan::Plan;
  [[nodiscard]] std::chrono::minutes planned_normal_working_time(const QDate& date) const noexcept override;
};

template<> struct fmt::formatter<Plan::Kind> : formatter<std::string>
{
  [[nodiscard]] static auto format(Plan::Kind kind, format_context& ctx)
  {
    const auto str = [kind]() {
      switch (kind) {
        using enum Plan::Kind;
      case Normal:
        return "Normal";
      case Sick:
        return "Sick";
      case Vacation:
        return "Vacation";
      case Holiday:
        return "Holiday";
      case HalfHoliday:
        return "Half Holiday";
      case HalfVacation:
        return "Half Vacation";
      case HalfVacationHalfHoliday:
        return "Half Vacation, Half Holiday";
      }
      Q_UNREACHABLE();
    }();
    using std::chrono_literals::operator""min;
    return fmt::format_to(ctx.out(), "{}", str);
  }
};

std::optional<std::vector<std::unique_ptr<Plan::Entry>>::const_iterator>
find_period_insert_pos(const std::vector<std::unique_ptr<Plan::Entry>>& periods, const Period& candidate) noexcept;
