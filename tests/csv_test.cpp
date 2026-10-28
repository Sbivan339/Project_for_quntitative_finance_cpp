#include "backtest/csv.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

TEST(Csv, ReadsPricesAndPreservesOrder) {
    std::istringstream input("close\n100\n101.5\n99\n");
    EXPECT_EQ(backtest::read_prices(input), (std::vector<double>{100, 101.5, 99}));
}

TEST(Csv, SupportsWhitespaceBlankRowsAndWindowsLineEndings) {
    std::istringstream input(" close\r\n 100 \r\n\r\n 1.5e2\r\n");
    EXPECT_EQ(backtest::read_prices(input), (std::vector<double>{100, 150}));
}

TEST(Csv, HeaderOnlyIsAnEmptySeries) {
    std::istringstream input("close\n");
    EXPECT_TRUE(backtest::read_prices(input).empty());
}

TEST(Csv, RejectsMissingOrIncorrectHeader) {
    for (const auto& text : {"", "price\n100\n", "100\n", "date,close\n"}) {
        std::istringstream input(text);
        EXPECT_THROW(backtest::read_prices(input), std::invalid_argument);
    }
}

TEST(Csv, RejectsBadPricesAndExtraColumns) {
    for (const auto& value : {"0", "-1", "nan", "inf", "oops", "100abc", "100,200",
                              "100 200", "1e9999"}) {
        std::istringstream input(std::string("close\n") + value + '\n');
        EXPECT_THROW(backtest::read_prices(input), std::invalid_argument);
    }
}

TEST(Csv, DetectsStreamFailure) {
    std::istringstream input("close\n100\n");
    input.setstate(std::ios::badbit);
    EXPECT_THROW(backtest::read_prices(input), std::invalid_argument);
}

} // namespace
