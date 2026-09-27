#pragma once

#include <QComboBox>

/**
 * @class ComboBox combobox.h "combobox.h"
 * @brief A QComboBox whose popup is as wide as its entries need.
 *
 * Qt gives the popup the width of the box itself, which for an editor inside a table cell is the
 * width of the column -- narrow enough to elide the very text the popup exists to choose
 * between. The box stays as wide as its cell; only the popup grows.
 */
class ComboBox : public QComboBox
{
  Q_OBJECT
public:
  using QComboBox::QComboBox;
  void showPopup() override;
};
