//Author:Movazed
//making the engine to excute our model + allocator queues for fast bidding......
// now after the core engine we are going to optimise it for realtime market and higher latency....
//let us now work on the last phase of live testing using boostios... //so we dont actually need to use custom datasets..
#include <iostream> // not using bits/stdc++ we dont want to process all the existing libraries...
#include <thread>
#include <vector>
#include <cstring>
#include <atomic>   // added for thread-safe shutdown flag
#include <deque>    // added for price history
#include <cmath>
#include <cstdlib>

#include "core/LockFreeQueue.hpp"
#include "core/ArenaAllocator.hpp"
#include "engine/OrderBook.hpp"
#include "strategies/AvellanedaStoikov.hpp"
#include "simdjson.h" // A fast JSON parser

#include <chrono> // we will use it for nano second timings....
#include <sched.h> // Required for CPU affinity


using namespace core;
using namespace engine;

// Global flag to signal when the network is done
std::atomic<bool> running{true};

//fixed packet structure for our LockFreeQueue
struct MarketMsg {
    char data[256];
    size_t length;
};

// change: adding rolling volatality
double calculate_volatility(const std::deque<double>& prices) {
    if (prices.size() < 2) return 0.0;

    double mean = 0;

    for (double p : prices) mean += p;
    mean /= prices.size();

    double variance = 0;
    for (double p : prices) variance += (p - mean) * (p - mean);

    return std::sqrt(variance / prices.size());
}

//Building the Producer Network Now Thread 1...

void network_thread(LockFreeQueue<MarketMsg, 1024>& queue) {

    const char* raw_packets[] = {
        R"({"id":1, "price":100.0, "qty":10, "side":"sell"})",
        R"({"id":2, "price":100.1, "qty":5,  "side":"buy"})",   // making custom vulnerabilites....
        R"({"id":3, "price":101.5, "qty":20, "side":"sell"})", // volatility Spike
        R"({"id":4, "price":103.0, "qty":15, "side":"buy"})",  // big Jump
        R"({"id":5, "price":100.0, "qty":10, "side":"sell"})", // crash
        R"({"id":6, "price":99.0,  "qty":10, "side":"buy"})",
        nullptr
    };

    std::cout << "[NET] Network Connected. Listening..\n";

    for (int i = 0; raw_packets[i] != nullptr; ++i) {

        MarketMsg msg;
        size_t len = std::strlen(raw_packets[i]);

        std::memcpy(msg.data, raw_packets[i], len);  //fast memory copy into messsage structure....
        msg.length = len;

        //push to queue if not we yeild to OS to let consumer catch up....
        while (!queue.push(msg)) {
            std::this_thread::yield();
        }

        //stimulating network latency fake delay...
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Increased slightly to see output clearly
    }

    // Allow a small buffer time for consumer to finish before shutting down
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << " [NET] All packets sent... \n";
    running = false; // Signal strategy thread to stop
}

void strategy_thread(LockFreeQueue<MarketMsg, 1024>& queue) {

    //1MB arena, we allocate once here, no mallocs allowed inside loops...
    ArenaAllocator arena(1024 * 1024);
    OrderBook book(arena);

    // Initialize the new startegic logics.....
    strategy::AvellanedaStoikov strat;

    std::deque<double> price_history;
    int current_inventory = 0;

    simdjson::dom::parser parser;

    std::cout << "[START] Engine Ready. Benchmarking Latency.....\n";

    MarketMsg msg;

    //entering the stats..
    long long total_latency = 0;
    int count = 0;
    long long max_latency = 0;
    long long min_latency = 99999999;

    // check the global atomic flag
    while (running) {

        if (queue.pop(msg)) {

            // we start the chrono clock here and measure strict wire to decision latency
            auto start_time = std::chrono::high_resolution_clock::now();

            simdjson::padded_string json(msg.data, msg.length);
            simdjson::dom::element doc;

            // json Parse AVX optimized AVX (Advanced Vector Extensions) optimized code leverages powerful SIMD ,Single Instruction,
            // multiple Data instructions on Intel/AMD CPUs to perform the same operation on multiple data elements in parallel        

            if (parser.parse(json).get(doc) == simdjson::SUCCESS) { //Extract Feild

                uint64_t id = doc["id"];
                double price = doc["price"];
                double qty = doc["qty"];
                std::string_view side_str = doc["side"];

                Side side = (side_str == "buy") ? Side::BUY : Side::SELL; //Now we process the orders

                // updating th orderbook....
                book.add_order(id, price, qty, side);

                //updating the history and inventory
                if (side == Side::BUY) current_inventory++;
                else current_inventory--;

                price_history.push_back(price);
                if (price_history.size() > 50) price_history.pop_front();

                // We calculate volatility from the history we just tracked
                double vol = calculate_volatility(price_history);

                // asking the ai brain or our model to quote
                auto quote = strat.calculate_quote(price, current_inventory, vol);

                //stopping the timer
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
                long long ns = duration.count();

                //updating of the latency
                total_latency += ns;
                count++;

                if (ns > max_latency) max_latency = ns;
                if (ns < min_latency) min_latency = ns;

                std::cout << "[LATENCY] " << ns << " ns"
                          << " | Action: " << (quote.bid_price > 0 ? "QUOTE" : "WAIT")
                          << "\n";

                // making logics for ai decision---->

                std::cout << "[STRAT] Vol:" << vol
                          << " | Inv:" << current_inventory
                          << (quote.panic_mode ? " | [PANIC MODE]" : " | [NORMAL]")
                          << " | Bid: " << quote.bid_price
                          << " Ask: " << quote.ask_price << "\n";
            }

        } else {

            //using relax function to reeduce thermal throtlling on core , this instuction is critical for low latency spin loops
            #if defined(__x86_64__) || defined(_M_X64)
                asm volatile("pause");
            #endif
        }
    }

    // reporting logics for measturments.... 

    if (count > 0) {

        std::cout << "\n========================================\n";
        std::cout << "       Latency Benchmark Reports         \n";
        std::cout << "\n========================================\n";
        std::cout << " total Orders: " << count << "\n";
        std::cout << " avg latency: " << (total_latency / count) << " ns\n";
        std::cout << " min latency: " << min_latency << " ns\n";
        std::cout << " max latency: " << max_latency << " ns\n";
        std::cout << "========================================\n";
    }
}

int main() {

    // the Lock Free-Queue on the stack -> fastest memory
    LockFreeQueue<MarketMsg, 1024> queue;

    //launching threads..
    std::thread producer(network_thread, std::ref(queue));
    std::thread consumer(strategy_thread, std::ref(queue));

    producer.join(); // wait for network to finish
    consumer.join(); // wait for strategy to finish (replaces detach to prevent segfault)

    return 0;
}

// hwo to compile : g++ src/engine/main.cpp simdjson.cpp -o hft_engine -O3 -march=native -pthread -I./include -I./src -Wno-interference-size
//./hft_engine
