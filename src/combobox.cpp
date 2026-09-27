#include "combobox.h"

#include <QAbstractItemView>
#include <QScreen>
#include <QStyle>
#include <algorithm>

void ComboBox::showPopup()
{
  auto& popup = *view();
  // sizeHintForColumn asks the delegate about every entry, so this covers whatever the widest one
  // needs, item margins included. The frame sits outside that, and so does the scroll bar, which
  // appears once the entries outgrow the popup. Capped at the screen, because a minimum width is
  // a floor Qt will not push back below when it fits the popup to the display.
  const auto scroll_bar_width = count() > maxVisibleItems() ? style()->pixelMetric(QStyle::PM_ScrollBarExtent) : 0;
  const auto wanted = popup.sizeHintForColumn(0) + (2 * popup.frameWidth()) + scroll_bar_width;
  popup.setMinimumWidth(std::min(wanted, screen()->availableGeometry().width()));
  QComboBox::showPopup();
}
