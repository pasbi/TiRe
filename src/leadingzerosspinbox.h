#pragma once

#include <QSpinBox>

class LeadingZerosSpinBox : public QSpinBox
{
public:
  using QSpinBox::QSpinBox;
  [[nodiscard]] QString textFromValue(int val) const override;
};
