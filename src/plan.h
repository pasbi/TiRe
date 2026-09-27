#pragma once

#include "application.h"
#include "db/entityid.h"
#include "fmt.h"
#include "period.h"

#include <QAbstractTableModel>
#include <QDate>
#include <array>
#include <chrono>
#include <ranges>

class AbstractTimeSheetRepository;
class IntervalModel;
class QDate;

class Plan : public QAbstractTableModel
{
  Q_OBJECT
public:
  static constexpr auto period_column = 0;
  static constexpr auto kind_column = 1;
  static constexpr auto share_column = 2;

  /**
   * @brief A kind of leave.
   *
   * Only leave is enumerated here: a day nobody booked anything on is an ordinary working day, so
   * "normal" is the absence of an entry rather than a kind of one.
   *
   * Two mechanisms live in this enum, and confusing them is the easiest way to get the overtime
   * balance wrong:
   * - Holiday and Vacation are a **budget that is spent**. Booking half a day consumes half a day
   *   whether or not you end up working, so the target simply drops by that much and every hour
   *   beyond it is genuine overtime.
   * - Sick and ChildSick are a **waiver that is claimed**, capped at the hours actually missed.
   *   Working through part of a sick day uses up only the part that was missed, so sickness can
   *   never earn overtime -- but it can never cost anything either.
   * @see planned_working_time(), leave_time()
   */
  enum class Kind { Sick, ChildSick, Holiday, Vacation };
  /** @brief Number of enumerators in Kind, so the editor cannot drift out of sync with it. */
  static constexpr auto kind_count = 4;

  /** @brief Whether @p kind spends a budget rather than waiving the target. @see Kind */
  [[nodiscard]] static constexpr bool is_budget(const Kind kind) noexcept
  {
    return kind == Kind::Holiday || kind == Kind::Vacation;
  }
  /** @brief Whether @p kind waives the target rather than spending a budget. @see Kind */
  [[nodiscard]] static constexpr bool is_waiver(const Kind kind) noexcept
  {
    return !is_budget(kind);
  }

  /**
   * @class Share plan.h "plan.h"
   * @brief The fraction of a day's normal working time that one booking claims.
   *
   * An entry covers a period rather than a single day, and a day's normal working time varies --
   * a weekend day is worth nothing, and a part-time plan may weigh weekdays differently. A leave
   * *duration* would therefore be right on the day it was chosen for and wrong on every other
   * one, while a fraction is right on all of them: half of a weekend day is still nothing.
   *
   * Held in permille rather than as a double, so that a half day survives a round trip through
   * the database exactly, and two of them add up to exactly a whole one.
   */
  class Share
  {
  public:
    static constexpr auto permille_per_whole = 1000;
    constexpr Share() noexcept = default;
    [[nodiscard]] static constexpr Share from_permille(const int permille) noexcept
    {
      return Share{permille};
    }
    [[nodiscard]] static constexpr Share from_percent(const int percent) noexcept
    {
      return Share{percent * permille_per_whole / 100};
    }
    [[nodiscard]] static constexpr Share none() noexcept
    {
      return Share{0};
    }
    [[nodiscard]] static constexpr Share whole() noexcept
    {
      return Share{permille_per_whole};
    }
    [[nodiscard]] constexpr int permille() const noexcept
    {
      return m_permille;
    }
    /** @brief This share of @p duration, rounded down to the minute. */
    [[nodiscard]] constexpr std::chrono::minutes of(const std::chrono::minutes duration) const noexcept
    {
      return duration * m_permille / permille_per_whole;
    }
    [[nodiscard]] friend constexpr Share operator+(const Share a, const Share b) noexcept
    {
      return Share{a.m_permille + b.m_permille};
    }
    [[nodiscard]] friend constexpr auto operator<=>(const Share&, const Share&) noexcept = default;

  private:
    explicit constexpr Share(const int permille) noexcept : m_permille(permille)
    {
    }
    int m_permille = 0;
  };

  /**
   * @class DayShares plan.h "plan.h"
   * @brief How much of one day each Kind claims.
   *
   * Several entries may cover the same day -- half a bank holiday and half a vacation day, say --
   * so everything downstream of the entries works on their sum. The entries themselves only
   * matter for editing.
   */
  class DayShares
  {
  public:
    [[nodiscard]] Share operator[](Kind kind) const noexcept;
    void add(Kind kind, Share share) noexcept;
    /** @brief What the day spends from a budget: holiday plus vacation. */
    [[nodiscard]] Share budget() const noexcept;
    /** @brief What the day may waive from its target: own plus child sickness. */
    [[nodiscard]] Share waiver() const noexcept;
    /** @brief Whether the day carries no leave at all, which makes it an ordinary working day. */
    [[nodiscard]] bool empty() const noexcept;

  private:
    std::array<Share, kind_count> m_shares{};
  };

  /**
   * @brief Why a booking cannot be made.
   *
   * The plan accepts any combination of leave that a day can physically hold, including ones
   * nobody would choose on purpose; only what cannot be accounted for is refused here. Offering
   * the *sensible* combinations is the editor's job, not this one's.
   */
  enum class Conflict {
    None,
    /** @brief The day would hold more leave than it is long. */
    Overbooked,
    /** @brief Own and child sickness on one day: the hours missed could not be told apart. */
    MixedSickness,
    /** @brief A share outside (0, 1], which no day can hold. */
    ImpossibleShare,
  };

  struct Entry
  {
    Period period;
    Kind kind = Kind::Vacation;
    Share share = Share::whole();
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

  /**
   * @name Leave lookup
   * The leave booked on a single day, or on each day of a period in turn.
   * @{
   */
  [[nodiscard]] DayShares shares_on(const QDate& date) const;
  [[nodiscard]] std::vector<DayShares> shares_in(const Period& period) const;
  /** @} */

  /**
   * @brief return a period which is free of leave.
   *  Return the period of today or of the first later day that carries no leave yet.
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
   *
   * @throws RuntimeError if the change would conflict with the rest of the plan. Check with the
   * matching check_*() first; a command whose redo() throws leaves the undo stack holding a step
   * that never applied.
   * @{
   */
  Kind swap_kind(const Entry& entry, Kind kind);
  Share swap_share(const Entry& entry, Share share);
  Period swap_period(const Entry& entry, Period period);
  /** @} */

  /**
   * @name Booking checks
   * Whether a change would leave every day of the plan accountable, and if not, why not.
   * check_add() asks about an entry that is not part of the plan yet; the others ask about
   * replacing one field of an entry that is.
   * @{
   */
  [[nodiscard]] Conflict check_add(const Entry& candidate) const;
  [[nodiscard]] Conflict check_kind(const Entry& entry, Kind kind) const;
  [[nodiscard]] Conflict check_share(const Entry& entry, Share share) const;
  [[nodiscard]] Conflict check_period(const Entry& entry, const Period& period) const;
  /** @} */

  /**
   * @brief How much leave of @p kind @p period contains.
   *
   * The two families of Kind deliberately count differently. Holiday and vacation report the
   * budget that was *spent*: a share of the normal working time, regardless of what was worked.
   * The sicknesses report the time actually *waived* -- the hours the waiver had to cover because
   * they were not worked -- so working six hours on a half child-sick day consumes two hours, not
   * four.
   *
   * That makes the sickness figures depend on @p interval_model, and it makes them shrink as work
   * is booked onto a sick day. It also keeps them honest as a remaining-allowance figure, which
   * is what child sickness needs: unlike own sickness it is capped by law, so the number on the
   * summary page is the point of the feature.
   */
  [[nodiscard]] std::chrono::minutes leave_time(Kind kind, const Period& period,
                                                const IntervalModel& interval_model) const;

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
  /**
   * @brief Whether @p candidate can be booked, ignoring @p replaced.
   * @p replaced is the entry @p candidate takes the place of, so that re-booking an entry does
   * not conflict with the booking it is about to replace. Pass nullptr for a new entry.
   */
  [[nodiscard]] Conflict check(const Entry& candidate, const Entry* replaced) const;
  [[nodiscard]] DayShares shares_on(const QDate& date, const Entry* excluded) const;
  [[nodiscard]] std::chrono::minutes planned_normal_working_time(const Period& period) const noexcept;
  [[nodiscard]] std::chrono::minutes planned_working_time(const QDate& date, const DayShares& shares,
                                                          const IntervalModel& interval_model) const noexcept;
  /**
   * @brief Sorts the periods by their beginning.
   * The periods are supposed to be sorted at any time, i.e., this function must only be called if
   * the ordering has been destroyed (e.g., after changing the beginning of a period in
   * m_periods). Entries may overlap, so equal beginnings are possible and keep their relative
   * order; unlike the ordering this sort cannot fail.
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
      case Sick:
        return "Sick";
      case ChildSick:
        return "Child Sick";
      case Holiday:
        return "Holiday";
      case Vacation:
        return "Vacation";
      }
      Q_UNREACHABLE();
    }();
    return fmt::format_to(ctx.out(), "{}", str);
  }
};

template<> struct fmt::formatter<Plan::Share> : formatter<std::string>
{
  [[nodiscard]] static auto format(const Plan::Share share, format_context& ctx)
  {
    // As a percentage, because that is how shares are entered and read; the permille resolution
    // exists for the arithmetic, not for the reader.
    static constexpr auto permille_per_percent = 10.0;
    return fmt::format_to(ctx.out(), "{}%", share.permille() / permille_per_percent);
  }
};

template<> struct fmt::formatter<Plan::DayShares> : formatter<std::string>
{
  [[nodiscard]] static auto format(const Plan::DayShares& shares, format_context& ctx)
  {
    auto out = ctx.out();
    auto separator = "";
    for (const auto i : std::views::iota(0, Plan::kind_count)) {
      const auto kind = static_cast<Plan::Kind>(i);
      if (shares[kind] == Plan::Share::none()) {
        continue;
      }
      out = fmt::format_to(out, "{}{} {}", separator, shares[kind], kind);
      separator = ", ";
    }
    return out;
  }
};
