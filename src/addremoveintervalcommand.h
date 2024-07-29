#pragma once

#include "command.h"

class Interval;
class IntervalModel;

class AddRemoveIntervalCommand : public Command
{
protected:
  explicit AddRemoveIntervalCommand(IntervalModel& interval_model, std::unique_ptr<Interval> interval);
  explicit AddRemoveIntervalCommand(IntervalModel& interval_model, const Interval& interval);
  ~AddRemoveIntervalCommand() override;
  void add();
  void remove();

private:
  IntervalModel& m_interval_model;
  std::unique_ptr<Interval> m_interval_owned;
  const Interval& m_interval_ref;
};

class AddIntervalCommand final : public AddRemoveIntervalCommand
{
public:
  explicit AddIntervalCommand(IntervalModel& interval_model, std::unique_ptr<Interval> interval);
  void undo() override;
  void redo() override;
};

class RemoveIntervalCommand final : public AddRemoveIntervalCommand
{
public:
  explicit RemoveIntervalCommand(IntervalModel& interval_model, const Interval& interval);
  void undo() override;
  void redo() override;
};
