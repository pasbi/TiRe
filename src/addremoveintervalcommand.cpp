#include "addremoveintervalcommand.h"
#include "intervalmodel.h"

AddRemoveIntervalCommand::AddRemoveIntervalCommand(IntervalModel& interval_model, std::unique_ptr<Interval> interval)
  : m_interval_model(interval_model), m_interval_owned(std::move(interval)), m_interval_ref(*m_interval_owned)
{
}

AddRemoveIntervalCommand::AddRemoveIntervalCommand(IntervalModel& interval_model, const Interval& interval)
  : m_interval_model(interval_model), m_interval_ref(interval)
{
}

AddRemoveIntervalCommand::~AddRemoveIntervalCommand() = default;

void AddRemoveIntervalCommand::remove()
{
  m_interval_owned = m_interval_model.extract_interval(m_interval_ref);
}

void AddRemoveIntervalCommand::add()
{
  m_interval_model.add_interval(std::move(m_interval_owned));
}

AddIntervalCommand::AddIntervalCommand(IntervalModel& interval_model, std::unique_ptr<Interval> interval)
  : AddRemoveIntervalCommand(interval_model, std::move(interval))
{
  setText(QObject::tr("Add Interval"));
}

void AddIntervalCommand::undo()
{
  remove();
}
void AddIntervalCommand::redo()
{
  add();
}

RemoveIntervalCommand::RemoveIntervalCommand(IntervalModel& interval_model, const Interval& interval)
  : AddRemoveIntervalCommand(interval_model, interval)
{
  setText(QObject::tr("Add Interval"));
}

void RemoveIntervalCommand::undo()
{
  add();
}

void RemoveIntervalCommand::redo()
{
  remove();
}
