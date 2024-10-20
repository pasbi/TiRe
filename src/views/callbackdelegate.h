#pragma once

#include <QStyledItemDelegate>

/**
 * @class The CallbackDelegate allows to call a callback instead of open an editor.
 * The callback could, e.g., spawn an editor.
 * This functionality could also be implemented by connection to the `QAbstractItemView::doubleClicked`-signal.
 * However, that would less elegant because:
 *  - connections and delegates would both share responsibility for creating editors
 *  - the model is required not to return the ItemIsEditable-flag to avoid creating the default in-place editor widget
 *    on double click. That's counterintuitive because the field is actually editable.
 */
class CallbackDelegate : public QStyledItemDelegate
{
public:
  explicit CallbackDelegate(std::function<void(const QModelIndex&)> edit_callback, QObject* parent = nullptr);
  QWidget* createEditor(QWidget* const parent, const QStyleOptionViewItem&, const QModelIndex& index) const override;

private:
  std::function<void(const QModelIndex&)> m_edit_callback;
};
