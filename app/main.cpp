#include "backtest/backtester.hpp"
#include "backtest/csv.hpp"

#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: backtest_cli [prices.csv]\n";
        return 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Usage: backtest_cli [prices.csv]\n"
                  << "CSV format: close header, then one positive price per row.\n"
                  << "Without a file, runs a built-in demo.\n";
        return 0;
    }
    try {
        std::vector<double> prices{100, 101, 102, 103, 104, 105, 106, 105,
                                   103, 101, 100, 102, 104, 106, 108, 110};
        if (argc == 2) {
            std::ifstream input(argv[1]);
            if (!input) {
                std::cerr << "Cannot open price file: " << argv[1] << '\n';
                return 1;
            }
            prices = backtest::read_prices(input);
        }
        const auto result = backtest::run(prices);
        std::cout << std::fixed << std::setprecision(2)
                  << "Prices: " << prices.size() << '\n'
                  << "Final equity: " << result.final_equity << '\n'
                  << "Total return: " << result.total_return * 100.0 << "%\n"
                  << "Maximum drawdown: " << result.max_drawdown * 100.0 << "%\n"
                  << "Executed fills: " << result.trades.size() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
