#pragma once

#include "combobox.h"

template<typename Enum, int enum_size> class EnumComboBox : public ComboBox
{
public:
  explicit EnumComboBox(QWidget* const parent = nullptr) : ComboBox(parent)
  {
    for (int i = 0; i < enum_size; ++i) {
      addItem(QString::fromStdString(fmt::format("{}", static_cast<Enum>(i))));
    }
  }

  void set_current_enum(const Enum value)
  {
    setCurrentIndex(static_cast<int>(value));
  }

  [[nodiscard]] Enum current_enum() const
  {
    return static_cast<Enum>(currentIndex());
  }
};
