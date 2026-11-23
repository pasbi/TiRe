#include "tableview.h"

#include <QApplication>
#include <QHeaderView>
#include <QStandardItemModel>
#include <array>
#include <gtest/gtest.h>

namespace
{

/** @brief QTableView keeps the content width of a column to itself; the test needs to see it. */
class ProbeTableView : public TableView
{
public:
  using TableView::sizeHintForColumn;
};

class TableViewTest : public ::testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    static auto arg0 = std::string{"tire-test"};
    static std::array argv{arg0.data(), static_cast<char*>(nullptr)};
    static auto argc = 1;
    static auto* const application = new QApplication{argc, argv.data()};
    Q_UNUSED(application)
  }

  /** @brief A table whose first column has far more to show than the other two. */
  void show_table(const QString& first_cell)
  {
    m_model.setRowCount(1);
    m_model.setColumnCount(3);
    m_model.setItem(0, 0, new QStandardItem{first_cell});
    m_model.setItem(0, 1, new QStandardItem{QStringLiteral("9:00")});
    m_model.setItem(0, 2, new QStandardItem{QStringLiteral("17:00")});
    m_view.setModel(&m_model);
    m_view.show();
    m_view.resize(800, 400);
    QApplication::processEvents();
  }

  [[nodiscard]] int column_slack(const int column) const
  {
    return m_view.columnWidth(column) - m_view.sizeHintForColumn(column);
  }

  QStandardItemModel m_model;
  ProbeTableView m_view;
};

}  // namespace

TEST_F(TableViewTest, EveryColumnIsWideEnoughForItsContent)
{
  show_table(QStringLiteral("A project whose name is far longer than any time of day"));

  for (int column = 0; column < m_model.columnCount(); ++column) {
    EXPECT_GE(m_view.columnWidth(column), m_view.sizeHintForColumn(column))
        << "column " << column << " cuts its content short";
  }
}

TEST_F(TableViewTest, TheWidestColumnTakesTheLargestShareOfTheSlack)
{
  show_table(QStringLiteral("A project whose name is far longer than any time of day"));

  EXPECT_GT(column_slack(0), column_slack(1)) << "the column with the most to show must grow the most";
  EXPECT_GT(m_view.columnWidth(0), m_view.columnWidth(1));
}

TEST_F(TableViewTest, TheColumnsFillTheViewport)
{
  show_table(QStringLiteral("short"));

  auto total = 0;
  for (int column = 0; column < m_model.columnCount(); ++column) {
    total += m_view.columnWidth(column);
  }
  EXPECT_EQ(m_view.maximumViewportSize().width(), total) << "no gap at the right edge";
}

TEST_F(TableViewTest, ColumnsGrowWhenTheContentDoes)
{
  show_table(QStringLiteral("short"));
  const auto narrow_width = m_view.columnWidth(0);

  m_model.setItem(0, 0, new QStandardItem{QStringLiteral("A project whose name is far longer than any time of day")});
  QApplication::processEvents();

  EXPECT_GT(m_view.columnWidth(0), narrow_width) << "a longer name must widen its column";
  EXPECT_GE(m_view.columnWidth(0), m_view.sizeHintForColumn(0));
}
