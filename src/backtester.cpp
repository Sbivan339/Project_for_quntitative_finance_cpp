#include "backtest/backtester.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace backtest {
namespace {

double as_double(long double value) {
    if (!std::isfinite(value) ||
        std::abs(value) > std::numeric_limits<double>::max()) {
        throw std::overflow_error("Portfolio value exceeds the double range");
    }
    return static_cast<double>(value);
}

void validate(const std::vector<double>& prices, const Config& config) {
    if (!std::isfinite(config.initial_cash) || config.initial_cash <= 0.0) {
        throw std::invalid_argument("Initial cash must be positive and finite");
    }
    if (!std::isfinite(config.commission_rate) || config.commission_rate < 0.0 ||
        config.commission_rate >= 1.0) {
        throw std::invalid_argument("Commission rate must be in [0, 1)");
    }
    if (config.fast_window == 0 || config.fast_window >= config.slow_window) {
        throw std::invalid_argument("Windows must satisfy 0 < fast < slow");
    }
    for (const double price : prices) {
        if (!std::isfinite(price) || price <= 0.0) {
            throw std::invalid_argument("Every price must be positive and finite");
        }
    }
}

} // namespace

Result run(const std::vector<double>& prices, const Config& config) {
    validate(prices, config);
    Result result;
    result.final_equity = config.initial_cash;
    result.equity_curve.reserve(prices.size());

    long double cash = config.initial_cash;
    long double quantity = 0.0L;
    long double peak = cash;
    long double fast_sum = 0.0L;
    long double slow_sum = 0.0L;
    const long double fee_rate = config.commission_rate;
    bool target_long = false;

    for (std::size_t i = 0; i < prices.size(); ++i) {
        const long double price = prices[i];

        // Execute yesterday's decision before observing today's closing price.
        if (target_long && quantity == 0.0L) {
            const long double bought = (cash / price) / (1.0L + fee_rate);
            const long double fee = bought * price * fee_rate;
            result.trades.push_back(
                {i, Side::Buy, prices[i], as_double(bought), as_double(fee)});
            quantity = bought;
            cash = 0.0L;
        } else if (!target_long && quantity > 0.0L) {
            const long double gross = quantity * price;
            const long double fee = gross * fee_rate;
            result.trades.push_back(
                {i, Side::Sell, prices[i], as_double(quantity), as_double(fee)});
            cash = gross - fee;
            quantity = 0.0L;
        }

        const long double equity = cash + quantity * price;
        result.equity_curve.push_back(as_double(equity));
        peak = std::max(peak, equity);
        result.max_drawdown = std::max(result.max_drawdown, as_double((peak - equity) / peak));

        // Maintain both sums incrementally rather than rescanning each window.
        fast_sum += price;
        slow_sum += price;
        if (i >= config.fast_window) {
            fast_sum -= prices[i - config.fast_window];
        }
        if (i >= config.slow_window) {
            slow_sum -= prices[i - config.slow_window];
        }
        if (i >= config.slow_window - 1) {
            target_long = fast_sum / config.fast_window > slow_sum / config.slow_window;
        }
    }

    if (!result.equity_curve.empty()) {
        result.final_equity = result.equity_curve.back();
    }
    result.total_return = as_double(
        static_cast<long double>(result.final_equity) / config.initial_cash - 1.0L);
    return result;
}

} // namespace backtest
