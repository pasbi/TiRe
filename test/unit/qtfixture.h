#pragma once

#include <QCoreApplication>
#include <gtest/gtest.h>

/**
 * @class QtFixture qtfixture.h "qtfixture.h"
 * @brief Provides the QCoreApplication that the Qt SQL drivers need.
 *
 * The drivers are Qt plugins, and plugin lookup requires a QCoreApplication instance. Without
 * one, QSqlDatabase::addDatabase fails with "driver not loaded" even though the driver is
 * installed.
 *
 * The instance is created once per test binary and intentionally never destroyed: tearing a
 * QCoreApplication down while gtest still holds static state is not worth the risk, and the
 * process is about to exit anyway.
 */
class QtFixture : public ::testing::Test
{
public:
  static void SetUpTestSuite()
  {
    static auto argc = 1;
    static std::array argv{const_cast<char*>("tire-test"), static_cast<char*>(nullptr)};
    static auto* const application = new QCoreApplication{argc, argv.data()};
    Q_UNUSED(application)
  }
};
