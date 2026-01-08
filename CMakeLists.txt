cmake_minimum_required(VERSION 3.10)
project(HFT_Engine)

# Use C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization Flags (The "Secret Sauce")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -pthread -Wall")
find_package(simdjson REQUIRED)
find_package(Threads REQUIRED)
include_directories(include)
add_executable(hft_engine src/engine/main.cpp)
target_link_libraries(hft_engine PRIVATE simdjson::simdjson Threads::Threads)