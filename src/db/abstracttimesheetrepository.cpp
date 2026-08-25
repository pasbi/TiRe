#include "db/abstracttimesheetrepository.h"

namespace
{

class NullTimeSheetRepository final : public AbstractTimeSheetRepository
{
  void insert(Project&) override
  {
  }
  void update(const Project&) override
  {
  }
  void remove(const Project&) override
  {
  }
  void insert(Interval&) override
  {
  }
  void update(const Interval&) override
  {
  }
  void remove(const Interval&) override
  {
  }
  void insert(Plan::Entry&) override
  {
  }
  void update(const Plan::Entry&) override
  {
  }
  void remove(const Plan::Entry&) override
  {
  }
  void update_plan_setting(const Plan&) override
  {
  }
};

}  // namespace

AbstractTimeSheetRepository::~AbstractTimeSheetRepository() = default;

AbstractTimeSheetRepository& null_repository() noexcept
{
  static NullTimeSheetRepository repository;
  return repository;
}
