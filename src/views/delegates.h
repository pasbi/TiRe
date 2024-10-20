#pragma once
#include <QComboBox>
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
template<typename BaseDelegate = QStyledItemDelegate> class CallbackDelegate : public BaseDelegate
{
public:
  CallbackDelegate(std::function<void(const QModelIndex&)> edit_callback, QObject* parent = nullptr)
    : BaseDelegate(parent), m_edit_callback(std::move(edit_callback))
  {
  }

  QWidget* createEditor(QWidget* const, const QStyleOptionViewItem&, const QModelIndex& index) const override
  {
    if (m_edit_callback) {
      m_edit_callback(index);
    }
    return nullptr;
  }

private:
  std::function<void(const QModelIndex&)> m_edit_callback;
};

template<typename Enum, typename BaseDelegate = QStyledItemDelegate> class KindDelegate : public BaseDelegate
{
  QWidget* createEditor(QWidget* const parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
  {
    auto cb = std::make_unique<QComboBox>(parent);
    for (int i = 0; i < 7; ++i) {
      cb->addItem(QString::fromStdString(fmt::format("{}", static_cast<Enum>(i))));
    }
    return cb.release();
  }

  void setEditorData(QWidget* const editor, const QModelIndex& index) const override
  {

    // const auto kind = static_cast<const Plan&>(*index.model()).entry(index.row()).kind;
    // static_cast<QComboBox*>(editor)->setCurrentIndex(static_cast<int>(kind));
  }

  void setModelData(QWidget* const editor, QAbstractItemModel* const model, const QModelIndex& index) const override
  {
    // const auto kind = static_cast<Plan::Kind>(static_cast<const QComboBox&>(*editor).currentIndex());
    // static_cast<Plan&>(*model).set_data(index.row(), kind);
  }
};
