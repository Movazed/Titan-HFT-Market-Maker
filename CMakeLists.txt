cmake_minimum_required(VERSION 3.14) # Changed to 3.14 to support Python3
project(HFT_Engine)

# keeping our High-Performance Standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Required for Python modules

# keeping our Optimization Flags ("The Secret Sauce")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -pthread -Wall")

include_directories(include)

# fetch dependencies

# find simdjson 
find_package(simdjson REQUIRED)

# find threads 
find_package(Threads REQUIRED)

# find OpenSSL (Required for boost::asio::ssl)
find_package(OpenSSL REQUIRED)

# fetching PyBind11 (The Bridge)
include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.10.0 
)
FetchContent_MakeAvailable(pybind11)

# finding python it is required for the bridge
# changed 'PythonComponents' to 'Python3'
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)


# target 1: the standalone engine (C++ Only)

add_executable(hft_engine src/engine/main.cpp)
# Link your existing high-speed libraries AND OpenSSL
target_link_libraries(hft_engine PRIVATE simdjson::simdjson Threads::Threads OpenSSL::SSL OpenSSL::Crypto)


# target 2: the python bridge 
# this creates 'hft_core.so' that Python can import

pybind11_add_module(hft_core 
    src/bindings.cpp 
)

# the strategy logic in bindings.cpp is pure math, so it doesn't need simdjson.

# source ~/HFT_Engine/Quant_Research/venv/bin/activate activation of env