#pragma once

#include "command.h"

template<typename Model, typename Item> class AddRemoveCommand : public Command
{
protected:
  explicit AddRemoveCommand(Model& model, std::unique_ptr<Item> item)
    : m_model(model), m_item_owned(std::move(item)), m_item_ref(*m_item_owned)
  {
  }

  explicit AddRemoveCommand(Model& model, const Item& item) : m_model(model), m_item_ref(item)
  {
  }

  void add()
  {
    m_model.add(std::move(m_item_owned));
  }

  void remove()
  {
    m_item_owned = m_model.extract(m_item_ref);
  }

private:
  Model& m_model;
  std::unique_ptr<Item> m_item_owned;
  const Item& m_item_ref;
};

template<typename Model, typename Item> class AddCommand final : public AddRemoveCommand<Model, Item>
{
public:
  explicit AddCommand(Model& model, std::unique_ptr<Item> item) : AddRemoveCommand<Model, Item>(model, std::move(item))
  {
  }

  void undo() override
  {
    this->remove();
  }

  void redo() override
  {
    this->add();
  }
};

template<typename Model, typename Item> class RemoveCommand final : public AddRemoveCommand<Model, Item>
{
public:
  explicit RemoveCommand(Model& model, const Item& item) : AddRemoveCommand<Model, Item>(model, item)
  {
  }

  void undo() override
  {
    this->add();
  }

  void redo() override
  {
    this->remove();
  }
};

template<template<typename...> typename CommandT, typename... Args> auto make(Args&&... args)
{
  auto* command = new CommandT(std::forward<Args>(args)...);
  return std::unique_ptr<Command>{command};
}
