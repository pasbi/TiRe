#pragma once

#include "plan.h"

class Interval;
class Project;

/**
 * @class AbstractTimeSheetRepository abstracttimesheetrepository.h "db/abstracttimesheetrepository.h"
 * @brief Persists individual entities as they change.
 *
 * The models call into this on every mutation, so the store is always up to date and there is
 * nothing to save. Only the models talk to a repository -- leaf types such as Project and
 * Interval stay unaware of persistence.
 *
 * insert() takes a non-const reference because it assigns the entity's id when it does not have
 * one yet. An entity that already carries an id keeps it, which is what lets a removal undone by
 * a command restore the original row.
 */
class AbstractTimeSheetRepository
{
public:
  AbstractTimeSheetRepository() = default;
  virtual ~AbstractTimeSheetRepository();
  AbstractTimeSheetRepository(const AbstractTimeSheetRepository&) = delete;
  AbstractTimeSheetRepository& operator=(const AbstractTimeSheetRepository&) = delete;
  AbstractTimeSheetRepository(AbstractTimeSheetRepository&&) = delete;
  AbstractTimeSheetRepository& operator=(AbstractTimeSheetRepository&&) = delete;

  virtual void insert(Project& project) = 0;
  virtual void update(const Project& project) = 0;
  virtual void remove(const Project& project) = 0;

  virtual void insert(Interval& interval) = 0;
  virtual void update(const Interval& interval) = 0;
  virtual void remove(const Interval& interval) = 0;

  virtual void insert(Plan::Entry& entry) = 0;
  virtual void update(const Plan::Entry& entry) = 0;
  virtual void remove(const Plan::Entry& entry) = 0;

  /** @brief Stores the plan's start date and overtime offset (a single row). */
  virtual void update_plan_setting(const Plan& plan) = 0;
};

/**
 * @brief A repository that discards everything.
 *
 * This is the default for models constructed without a database, which keeps them usable in
 * tests and benchmarks without any SQL in sight.
 */
[[nodiscard]] AbstractTimeSheetRepository& null_repository() noexcept;
