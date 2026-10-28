cmake_minimum_required(VERSION 3.20)
project(TradingBacktester VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BACKTEST_BUILD_BENCHMARKS "Build Google Benchmark performance tests" ON)
option(BACKTEST_BUILD_DOCS "Add the Doxygen docs target" OFF)
include(CTest)

add_library(backtest src/backtester.cpp src/csv.cpp)
target_include_directories(backtest PUBLIC "${PROJECT_SOURCE_DIR}/include")
if(MSVC)
    target_compile_options(backtest PRIVATE /W4)
else()
    target_compile_options(backtest PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(backtest_cli app/main.cpp)
target_link_libraries(backtest_cli PRIVATE backtest)

if(BUILD_TESTING OR BACKTEST_BUILD_BENCHMARKS)
    include(FetchContent)
endif()

if(BUILD_TESTING)
    # Fixed releases make dependency upgrades an explicit change.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()

if(BACKTEST_BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(googlebenchmark)
    add_subdirectory(benchmarks)
endif()

if(BACKTEST_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    configure_file(docs/Doxyfile.in "${PROJECT_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(docs
        COMMAND "${DOXYGEN_EXECUTABLE}" "${PROJECT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Generating Doxygen HTML documentation"
        VERBATIM
    )
endif()
