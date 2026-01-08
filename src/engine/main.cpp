//Author:Movazed 

#include <iostream> //not using bits/stdc++ we dont want to process all the existing libraries...
#include <thread>
#include <vector>
#include <cstring>
#include <atomic> // Added for thread-safe shutdown flag
#include "core/LockFreeQueue.hpp"
#include "core/ArenaAllocator.hpp"
#include "engine/OrderBook.hpp"
#include "simdjson.h" // A fast JSON parser

using namespace core;
using namespace engine;

// Global flag to signal when the network is done
std::atomic<bool> running{true};

//fixed packet structure for our LockFreeQueue
struct MarketMsg {
    char data[256];
    size_t length;
};

//Building the Producer Network Now Thread 1...

void network_thread(LockFreeQueue<MarketMsg, 1024>& queue) {
    const char* raw_packets[] = {
        R"({"id":1, "price":100.0, "qty":10, "side":"sell"})",
        R"({"id":2, "price":101.0, "qty":20, "side":"sell"})",
        R"({"id":3, "price":100.0, "qty":15, "side":"buy"})",
        nullptr
    };

    std::cout << "[NET] Network Connected. Listening..\n";

    for(int i = 0;  raw_packets[i] != nullptr; ++i){
        MarketMsg msg;
        size_t len = std::strlen(raw_packets[i]); 

        std::memcpy(msg.data, raw_packets[i],len);  //fast memory copy into messsage structure....
        msg.length = len;           

        //push to queue if not we yeild to OS to let consumer catch up....

        while(!queue.push(msg)){
            std::this_thread::yield();
        }
            //stimulating network latency fake delay...
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Allow a small buffer time for consumer to finish before shutting down
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout<<" [NET] All packets sent... \n";
    running = false; // Signal strategy thread to stop
}

void strategy_thread(LockFreeQueue<MarketMsg, 1024>& queue) {
    //1MB arena, we allocate once here, no mallocs allowed inside loops...

    ArenaAllocator arena(1024 * 1024);
    OrderBook book(arena);
    simdjson::dom::parser parser;

    std::cout << "[START] Engine Ready.\n";
    
    MarketMsg msg;
    
    // Check the global atomic flag
    while(running){
        if(queue.pop(msg)){
            simdjson::padded_string json(msg.data, msg.length);
            simdjson::dom::element doc;
            //Json Parse AVX optimized AVX (Advanced Vector Extensions) optimized code leverages powerful SIMD ,Single Instruction,
            //  Multiple Data instructions on Intel/AMD CPUs to perform the same operation on multiple data elements in parallel
            
            if(parser.parse(json).get(doc) == simdjson::SUCCESS){ //Extract Feild
                uint64_t id = doc["id"];
                double price = doc["price"];
                double qty = doc["qty"];
                std::string_view side_str = doc["side"];

                Side side = (side_str == "buy") ? Side::BUY : Side::SELL; //Now we process the orders

                book.add_order(id, price, qty, side);
            }
        } else {
            //using relax function to reeduce thermal throtlling on core , this instuction is critical for low latency spin loops
            #if defined(__x86_64__) || defined(_M_X64)
                asm volatile("pause");
            #endif
        }
    }
}

int main()  {
    //The Lock Free-Queue on the stack -> fastest memory

    LockFreeQueue<MarketMsg, 1024> queue;
    
    //launching threads..
    std::thread producer(network_thread, std::ref(queue));
    std::thread consumer(strategy_thread, std::ref(queue));

    producer.join(); // wait for network to finish
    consumer.join(); // wait for strategy to finish (replaces detach to prevent segfault)

    return 0;
}