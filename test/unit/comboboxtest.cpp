#include "combobox.h"

#include <QAbstractItemView>
#include <QApplication>
#include <array>
#include <gtest/gtest.h>

namespace
{

class ComboBoxTest : public ::testing::Test
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
};

}  // namespace

TEST_F(ComboBoxTest, PopupIsAsWideAsItsWidestEntry)
{
  // How wide the popup ends up is the style's decision, and the styles disagree: Fusion, and with
  // it the offscreen platform this runs on, widens it by itself, while Breeze hands it the width
  // of the box -- which is what this class exists for. What can be pinned here is the floor it
  // puts under that decision: the view's minimum follows its widest entry rather than the box.
  ComboBox combo_box;
  combo_box.addItem(QStringLiteral("An entry far too long for the column this box sits in"));
  combo_box.addItem(QStringLiteral("Sick"));
  combo_box.resize(40, 20);
  combo_box.show();

  combo_box.showPopup();
  auto* const popup = combo_box.view();
  EXPECT_GE(popup->minimumWidth(), popup->sizeHintForColumn(0)) << "the widest entry must fit";
  EXPECT_GT(popup->minimumWidth(), combo_box.width()) << "the popup must not inherit the width of the box";
  combo_box.hidePopup();
}
