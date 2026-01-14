#!/bin/bash

trap "kill 0" EXIT

echo "=========================================="
echo "  Launching Titan Trading System (Cpp + web)    "
echo "=========================================="

# 1. COMPILE
echo "[1/3] Compiling C++ Engine..."
g++ src/engine/main.cpp simdjson.cpp -o hft_live \
    -O3 -march=native -pthread \
    -I./include -I./src \
    -lssl -lcrypto -lboost_system \
    -Wno-interference-size

if [ $? -ne 0 ]; then
    echo "Compilation Failed!"
    exit 1
fi

# starting the engine here
echo "[2/3] Starting HFT Engine in Background..."
# running silently on background so no load on machine giving errors of engine on logs on engine_log.txt
./hft_live > engine_logs.txt 2>&1 &
echo "      -> Engine running..."

# starting the dashboard here....
echo "[3/3] Firing up Streamlit Dashboard..."
sleep 2 # Wait for engine to create the CSV file
streamlit run app.py
