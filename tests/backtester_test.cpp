#include "backtest/backtester.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>
#include <vector>

namespace {

backtest::Config test_config() {
    return {1000.0, 0.0, 1, 2};
}

TEST(Backtester, EmptySeriesKeepsInitialCash) {
    const auto result = backtest::run({}, test_config());
    EXPECT_DOUBLE_EQ(result.final_equity, 1000.0);
    EXPECT_DOUBLE_EQ(result.total_return, 0.0);
    EXPECT_DOUBLE_EQ(result.max_drawdown, 0.0);
    EXPECT_TRUE(result.trades.empty());
    EXPECT_TRUE(result.equity_curve.empty());
}

TEST(Backtester, WarmupDoesNotTrade) {
    const auto result = backtest::run({100, 200}, test_config());
    EXPECT_TRUE(result.trades.empty());
    EXPECT_DOUBLE_EQ(result.final_equity, 1000.0);
}

TEST(Backtester, SignalExecutesAtNextPriceRatherThanSignalPrice) {
    const auto result = backtest::run({100, 110, 200, 220}, test_config());
    ASSERT_EQ(result.trades.size(), 1U);
    EXPECT_EQ(result.trades[0].index, 2U);
    EXPECT_EQ(result.trades[0].side, backtest::Side::Buy);
    EXPECT_DOUBLE_EQ(result.trades[0].price, 200.0);
    EXPECT_DOUBLE_EQ(result.trades[0].quantity, 5.0);
    EXPECT_DOUBLE_EQ(result.final_equity, 1100.0);
    EXPECT_NEAR(result.total_return, 0.1, 1e-12);
}

TEST(Backtester, FlatAndFallingPricesStayInCash) {
    for (const auto& prices : {std::vector<double>{100, 100, 100, 100},
                               std::vector<double>{100, 90, 80, 70}}) {
        const auto result = backtest::run(prices, test_config());
        EXPECT_TRUE(result.trades.empty());
        EXPECT_DOUBLE_EQ(result.final_equity, 1000.0);
    }
}

TEST(Backtester, SellExecutesOneStepAfterDownwardSignal) {
    const auto result = backtest::run({100, 110, 120, 100, 90}, test_config());
    ASSERT_EQ(result.trades.size(), 2U);
    EXPECT_EQ(result.trades[1].side, backtest::Side::Sell);
    EXPECT_EQ(result.trades[1].index, 4U);
    EXPECT_DOUBLE_EQ(result.trades[1].price, 90.0);
    EXPECT_NEAR(result.final_equity, 750.0, 1e-9);
    EXPECT_NEAR(result.max_drawdown, 0.25, 1e-12);
}

TEST(Backtester, ChargesBothBuyAndSellCommissions) {
    auto config = test_config();
    config.commission_rate = 0.01;
    const auto result = backtest::run({100, 110, 120, 100, 90}, config);
    ASSERT_EQ(result.trades.size(), 2U);
    const double quantity = 1000.0 / (120.0 * 1.01);
    EXPECT_NEAR(result.trades[0].quantity, quantity, 1e-12);
    EXPECT_NEAR(result.trades[0].commission, quantity * 120.0 * 0.01, 1e-12);
    EXPECT_NEAR(result.trades[1].commission, quantity * 90.0 * 0.01, 1e-12);
    EXPECT_NEAR(result.final_equity, quantity * 90.0 * 0.99, 1e-9);
}

TEST(Backtester, OpenPositionIsMarkedToMarketWithoutForcedExit) {
    auto config = test_config();
    config.commission_rate = 0.01;
    const auto result = backtest::run({100, 110, 120, 150}, config);
    ASSERT_EQ(result.trades.size(), 1U);
    EXPECT_NEAR(result.final_equity, 1000.0 / 1.01 / 120.0 * 150.0, 1e-9);
    EXPECT_NEAR(result.max_drawdown, 1.0 - 1.0 / 1.01, 1e-12);
}

TEST(Backtester, DrawdownUsesRunningPeak) {
    const auto result = backtest::run({100, 110, 120, 150, 130, 125}, test_config());
    EXPECT_NEAR(result.max_drawdown, 1.0 - 125.0 / 150.0, 1e-12);
}

TEST(Backtester, SubsequentRunsDoNotShareStrategyState) {
    const std::vector<double> prices{100, 110, 120, 100, 90};
    const auto first = backtest::run(prices, test_config());
    const auto second = backtest::run(prices, test_config());
    EXPECT_EQ(first.equity_curve, second.equity_curve);
    EXPECT_EQ(first.trades.size(), second.trades.size());
}

TEST(Backtester, FuturePricesDoNotChangePastEquityOrFills) {
    const auto prefix = backtest::run({100, 110, 120, 130}, test_config());
    const auto extended = backtest::run({100, 110, 120, 130, 1, 10000}, test_config());
    for (std::size_t i = 0; i < prefix.equity_curve.size(); ++i) {
        EXPECT_DOUBLE_EQ(prefix.equity_curve[i], extended.equity_curve[i]);
    }
    ASSERT_FALSE(prefix.trades.empty());
    ASSERT_FALSE(extended.trades.empty());
    EXPECT_EQ(prefix.trades[0].index, extended.trades[0].index);
    EXPECT_DOUBLE_EQ(prefix.trades[0].quantity, extended.trades[0].quantity);
}

TEST(Backtester, RejectsNonpositiveOrNonfinitePrices) {
    for (const double price : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::quiet_NaN()}) {
        EXPECT_THROW(backtest::run({100, price}, test_config()), std::invalid_argument);
    }
}

TEST(Backtester, RejectsInvalidWindowsEvenForEmptyInput) {
    auto config = test_config();
    config.fast_window = 0;
    EXPECT_THROW(backtest::run({}, config), std::invalid_argument);
    config.fast_window = config.slow_window;
    EXPECT_THROW(backtest::run({}, config), std::invalid_argument);
    config.fast_window = 3;
    EXPECT_THROW(backtest::run({}, config), std::invalid_argument);
}

TEST(Backtester, RejectsInvalidCashAndFeeRate) {
    for (const double cash : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                              std::numeric_limits<double>::quiet_NaN()}) {
        auto config = test_config();
        config.initial_cash = cash;
        EXPECT_THROW(backtest::run({}, config), std::invalid_argument);
    }
    for (const double fee : {-0.1, 1.0, std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::quiet_NaN()}) {
        auto config = test_config();
        config.commission_rate = fee;
        EXPECT_THROW(backtest::run({}, config), std::invalid_argument);
    }
}

TEST(Backtester, ReportsNumericOverflowInsteadOfInfiniteEquity) {
    auto config = test_config();
    config.initial_cash = std::numeric_limits<double>::max();
    EXPECT_THROW(backtest::run({1, 2, 3, 100}, config), std::overflow_error);
}

} // namespace
