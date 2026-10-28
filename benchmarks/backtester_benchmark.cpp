#include "backtest/backtester.hpp"

#include <benchmark/benchmark.h>

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

void backtest_series(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    std::vector<double> prices;
    prices.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        // Bounded deterministic data exercises both buys and sells.
        prices.push_back(100.0 + 10.0 * std::sin(static_cast<double>(i) * 0.05));
    }
    const backtest::Config config{10000.0, 0.001, 10, 30};
    for (auto _ : state) {
        auto result = backtest::run(prices, config);
        benchmark::DoNotOptimize(result.final_equity);
        benchmark::DoNotOptimize(result.trades.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(count));
}

BENCHMARK(backtest_series)->Arg(1000)->Arg(10000)->Arg(100000)->Unit(benchmark::kMicrosecond);

} // namespace
