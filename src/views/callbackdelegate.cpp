#include "views/callbackdelegate.h"

CallbackDelegate::CallbackDelegate(std::function<void(const QModelIndex&)> edit_callback, QObject* parent)
  : QStyledItemDelegate(parent), m_edit_callback(std::move(edit_callback))
{
}

QWidget* CallbackDelegate::createEditor(QWidget* const parent, const QStyleOptionViewItem& style_option_view_item,
                                        const QModelIndex& index) const
{
  if (m_edit_callback) {
    m_edit_callback(index);
  }
  return nullptr;
}
