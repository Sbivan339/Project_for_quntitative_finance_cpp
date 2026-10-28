#pragma once

#include <istream>
#include <vector>

/** @file csv.hpp
 * @brief Import a one-column historical price series.
 */

namespace backtest {

/**
 * @brief Read a CSV stream with the header `close` and one price per row.
 * @param input CSV text; surrounding whitespace and blank rows are permitted.
 * @return Positive finite prices, preserving row order.
 * @throws std::invalid_argument If the header or a nonblank data row is invalid.
 * @throws std::runtime_error On an input stream failure.
 */
std::vector<double> read_prices(std::istream& input);

} // namespace backtest
