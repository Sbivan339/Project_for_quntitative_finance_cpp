#include "backtest/csv.hpp"

#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

namespace backtest {
namespace {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

} // namespace

std::vector<double> read_prices(std::istream& input) {
    std::string line;
    if (!std::getline(input, line) || trim(line) != "close") {
        throw std::invalid_argument("CSV must start with the header: close");
    }
    std::vector<double> prices;
    std::size_t line_number = 1;
    while (std::getline(input, line)) {
        ++line_number;
        const auto value = trim(line);
        if (value.empty()) {
            continue;
        }
        std::istringstream row(value);
        row.imbue(std::locale::classic());
        double price = 0.0;
        if (!(row >> price) || !std::isfinite(price) || price <= 0.0) {
            throw std::invalid_argument("Invalid price on CSV line " + std::to_string(line_number));
        }
        row >> std::ws;
        if (!row.eof()) {
            throw std::invalid_argument("Extra data on CSV line " + std::to_string(line_number));
        }
        prices.push_back(price);
    }
    if (input.bad() || (input.fail() && !input.eof())) {
        throw std::runtime_error("Failed to read CSV stream");
    }
    return prices;
}

} // namespace backtest
