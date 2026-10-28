#pragma once

#include <cstddef>
#include <vector>

/** @file backtester.hpp
 * @brief Long-only backtesting of a moving-average strategy on a price series.
 */

namespace backtest {

/** @brief Configuration for a long-only, fully invested strategy. */
struct Config {
    double initial_cash = 10000.0; ///< Positive starting balance.
    double commission_rate = 0.001; ///< Proportional fee per fill, in [0, 1).
    std::size_t fast_window = 3; ///< Short moving-average window.
    std::size_t slow_window = 5; ///< Long window; must exceed fast_window.
};

/** @brief Direction of an executed fill. */
enum class Side { Buy, Sell };

/** @brief One order execution at a historical price. */
struct Trade {
    std::size_t index; ///< Index of the execution price, starting at zero.
    Side side; ///< Buy or sell.
    double price; ///< Execution price.
    double quantity; ///< Fractional units; short selling is not supported.
    double commission; ///< Cash paid as the fee for this fill.
};

/** @brief Portfolio statistics and the complete history of a run. */
struct Result {
    double final_equity = 0.0; ///< Cash plus the final marked-to-market position.
    double total_return = 0.0; ///< final_equity / initial_cash - 1, as a fraction.
    double max_drawdown = 0.0; ///< Largest peak-to-trough loss, as a fraction.
    std::vector<double> equity_curve; ///< Portfolio value after each price step.
    std::vector<Trade> trades; ///< Executed fills; not closed round trips.
};

/**
 * @brief Run a moving-average crossover strategy in O(n) time.
 *
 * The strategy requests a long position if the fast average is strictly above
 * the slow average, and cash otherwise. No signal is produced before the slow
 * window is full. A signal at index i executes at price i+1, never at price i.
 * Buys invest all cash, including fees. Sells liquidate the complete position.
 * Any position remaining at the end is marked to market without a forced sale.
 * Empty input returns the initial cash and an empty history.
 *
 * @param prices Positive, finite, equally spaced prices in chronological order.
 * @param config Windows, starting balance, and commission rate.
 * @return Equity history, fills, total return, and maximum drawdown.
 * @throws std::invalid_argument If prices or configuration are invalid.
 * @throws std::overflow_error If an accounting result cannot fit in a double.
 */
Result run(const std::vector<double>& prices, const Config& config = {});

} // namespace backtest
