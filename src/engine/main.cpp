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

// now last phase network threads..
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include "core/LockFreeQueue.hpp"
#include "core/ArenaAllocator.hpp"
#include "engine/OrderBook.hpp"
#include "strategies/AvellanedaStoikov.hpp"
#include "simdjson.h" // A fast JSON parser

#include <chrono> // we will use it for nano second timings....
#include <sched.h> // Required for CPU affinity

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

using namespace core;
using namespace engine;

// Global flag to signal when the network is done
std::atomic<bool> running{true};

//fixed packet structure for our LockFreeQueue
struct MarketMsg {
    char data[1024];
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

    // const char* raw_packets[] = {
    //     R"({"id":1, "price":100.0, "qty":10, "side":"sell"})",
    //     R"({"id":2, "price":100.1, "qty":5,  "side":"buy"})",   // making custom vulnerabilites....
    //     R"({"id":3, "price":101.5, "qty":20, "side":"sell"})", // volatility Spike
    //     R"({"id":4, "price":103.0, "qty":15, "side":"buy"})",  // big Jump
    //     R"({"id":5, "price":100.0, "qty":10, "side":"sell"})", // crash
    //     R"({"id":6, "price":99.0,  "qty":10, "side":"buy"})",
    //     nullptr
    // };

    try{
        std::cout << "[NET] Initializing Secure Connection to Binance...\n"; // we use io context which is required for all I/O
        net::io_context ioc;                            // the context is required for all I/o;
        ssl::context ctx{ssl::context::tlsv12_client};  
        ctx.set_verify_mode(ssl::verify_none);          // speed over strict verification...
        tcp::resolver resolver{ioc};                    //resolve domain..
        auto const results = resolver.resolve("stream.binance.com", "9443");
        websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc, ctx};
        net::connect(beast::get_lowest_layer(ws), results);
        
        //ssl handshake
        // beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(30));
        ws.next_layer().handshake(ssl::stream_base::client);
        
        //websockets handshake
        // beast::get_lowest_layer(ws).expires_never();
        ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

        //subscribe to BTC/USDT Trade System
        ws.handshake("stream.binance.com", "/ws/btcusdt@trade");
        std::cout << "[NET] Network Connected, listening to LIVE Stream..\n"; //for tracking the current trades
        beast::flat_buffer buffer;

        while(running){
            ws.read(buffer); // read a message into buffer
            //convert to string to copy (simdjson requires padding later...)
            // FIXED: buffer_to_string -> buffers_to_string
            std::string msg_str = beast::buffers_to_string(buffer.data());

            MarketMsg msg; //cap length to avoid buffer overflow
            size_t copy_len = std::min(msg_str.size(), sizeof(msg.data) - 1);

            std::memcpy(msg.data, msg_str.c_str(), copy_len); //fast memory copy
            msg.data[copy_len] = '\0';
            msg.length = copy_len;

            //push to queue if not we make the os to let consumer thread catch up....
            while(!queue.push(msg)){
                std::this_thread::yield(); // FIXED: yeild -> yield
            }

            // clear buffer to this message 
            buffer.consume(buffer.size());
        }
    } catch (std::exception const& e) {
        std::cerr << "[NET] Error: " << e.what() << std::endl;
        running = false;
    }
}
//     std::cout << "[NET] Network Connected. Listening..\n";

//     for (int i = 0; raw_packets[i] != nullptr; ++i) {

//         MarketMsg msg;
//         size_t len = std::strlen(raw_packets[i]);

//         std::memcpy(msg.data, raw_packets[i], len);  //fast memory copy into messsage structure....
//         msg.length = len;

//         //push to queue if not we yeild to OS to let consumer catch up....
//         while (!queue.push(msg)) {
//             std::this_thread::yield();
//         }

//         //stimulating network latency fake delay...
//         std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Increased slightly to see output clearly
//     }

//     // Allow a small buffer time for consumer to finish before shutting down
//     std::this_thread::sleep_for(std::chrono::seconds(1));

//     std::cout << " [NET] All packets sent... \n";
//     running = false; // Signal strategy thread to stop
// }

void strategy_thread(LockFreeQueue<MarketMsg, 1024>& queue) {

    //1MB arena, we allocate once here, no mallocs allowed inside loops...
    ArenaAllocator arena(1024 * 1024);
    OrderBook book(arena);

    // Initialize the new startegic logics.....
    strategy::AvellanedaStoikov strat;

    std::deque<double> price_history;
    int current_inventory = 0;

    simdjson::dom::parser parser;

    //warming up the benchmark
    for(int i=0; i<10000; i++) strat.calculate_quote(100.0, 0, 0.01);

    std::cout << "[START] Engine Ready. Benchmarking Latency.....\n";

    MarketMsg msg;

    //entering the stats..
    long long total_latency = 0;
    int count = 0;
    long long max_latency = 0;
    long long min_latency = 99999999;
    uint64_t trade_counter = 0;

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

                // "p": "Price", "q": "Qty", "m": isBuyerMaker , but binance is different from our fake data...
                try{
                    std::string_view p_str = doc["p"].get_string().value();
                    std::string_view q_str = doc["q"].get_string().value();
                    bool is_maker_buyer = doc["m"].get_bool();    
                    
                    double price =  std::strtod(p_str.data(), nullptr);
                    double qty = std::strtod(q_str.data(), nullptr); //strtod is fast

                    Side side = is_maker_buyer ? Side::SELL : Side::BUY; //after determining side we process orders

                    book.add_order(0,price, qty, side); //use ID 0 for live stream aggregation updating the orderbook...

                    if(side == Side::BUY) current_inventory++;
                    else current_inventory--;

                    price_history.push_back(price);
                    if(price_history.size() > 50) price_history.pop_front();

                    double vol  = calculate_volatility(price_history); //we calcualte the volatility..

                    // 'ask' -> 'auto'
                    auto quote = strat.calculate_quote(price, current_inventory, vol); // asking our ML model to quote....

                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time); // stopping the timer
                    long long ns = duration.count();

                    //latency update

                    total_latency += ns;
                    count++;
                    trade_counter++;

                    if (ns > max_latency) max_latency = ns;
                    if (ns < min_latency) min_latency = ns;

                    if (trade_counter % 50 == 0) {
                         std::cout << "[LIVE] BTC: " << price << " | Latency: " << ns << " ns\n"; //printing periodically to save the logs of IO
                    }
                } catch (...) {
                    // Ignore malformed packets (like ping/pong)
                }

                // uint64_t id = doc["id"];
                // double price = doc["price"];
                // double qty = doc["qty"];
                // std::string_view side_str = doc["side"];

                // Side side = (side_str == "buy") ? Side::BUY : Side::SELL; //Now we process the orders

                // // updating th orderbook....
                // book.add_order(id, price, qty, side);

                // //updating the history and inventory
                // if (side == Side::BUY) current_inventory++;
                // else current_inventory--;

                // price_history.push_back(price);
                // if (price_history.size() > 50) price_history.pop_front();

                // // We calculate volatility from the history we just tracked
                // double vol = calculate_volatility(price_history);

                // // asking the ai brain or our model to quote
                // auto quote = strat.calculate_quote(price, current_inventory, vol);

                // //stopping the timer
                // auto end_time = std::chrono::high_resolution_clock::now();
                // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
                // long long ns = duration.count();

                // //updating of the latency
                // total_latency += ns;
                // count++;

                // if (ns > max_latency) max_latency = ns;
                // if (ns < min_latency) min_latency = ns;

                // std::cout << "[LATENCY] " << ns << " ns"
                //           << " | Action: " << (quote.bid_price > 0 ? "QUOTE" : "WAIT")
                //           << "\n";

                // // making logics for ai decision---->

                // std::cout << "[STRAT] Vol:" << vol
                //           << " | Inv:" << current_inventory
                //           << (quote.panic_mode ? " | [PANIC MODE]" : " | [NORMAL]")
                //           << " | Bid: " << quote.bid_price
                //           << " Ask: " << quote.ask_price << "\n";
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

// hwo to compile : g++ src/engine/main.cpp simdjson.cpp -o hft_engine -O3 -march=native -pthread -I./include -I./src -lssl -lcrypto -lboost_system -Wno-interference-size
//./hft_engine
//./hft_live ->this for the new one after compiling