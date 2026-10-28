add_executable(backtest_tests backtester_test.cpp csv_test.cpp)
target_link_libraries(backtest_tests PRIVATE backtest GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(backtest_tests)

add_test(NAME cli_demo COMMAND backtest_cli)
add_test(NAME cli_csv COMMAND backtest_cli "${PROJECT_SOURCE_DIR}/data/prices.csv")
add_test(NAME cli_missing_file COMMAND backtest_cli "${CMAKE_CURRENT_BINARY_DIR}/missing.csv")
set_tests_properties(cli_missing_file PROPERTIES WILL_FAIL TRUE)
