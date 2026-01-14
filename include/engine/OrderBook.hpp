#pragma once
#include "core/ArenaAllocator.hpp"
#include <map>
#include <iostream>
#include <algorithm>

namespace engine {
    enum class Side: uint8_t {BUY, SELL};

    struct Order {
        uint64_t id;
        double price;
        double quantity;
        Side side;
        Order* next = nullptr;        
        // we are using intrusive pointers, we link orders together inside struct,avoid slowness of list node allocation
    };

class OrderBook{
private:
    std::map<double, Order*, std::greater<double>> bids_; //using greater for highest price first
    std::map<double, Order*> asks_; // Lowest price first

    core::ArenaAllocator& arena_;

public:
    explicit OrderBook(core::ArenaAllocator& arena) : arena_(arena){}

    OrderBook(const OrderBook&) = delete; // for safety no copying allowed
    OrderBook& operator=(const OrderBook&) = delete;

    void add_order(uint64_t id , double price, double qty, Side side){ //we are going zero latency allocation from our Arena
            Order* order = arena_.allocate<Order>();
            order->id = id;
            order->price = price;
            order->quantity = qty;
            order->side = side;
            order->next = nullptr;

            if(side == Side::BUY){
                match_order(order, asks_, bids_);  //making a route to match the logics.....
            } else {
                match_order(order, bids_, asks_);
            }
        }
private:
        template<typename BookMap, typename OppositeBookMap>
        void match_order(Order* incoming, BookMap& opposite_book, OppositeBookMap& same_side_book) { //Templated Logic for buy vs ask and sell vs bids
            auto it = opposite_book.begin(); // while incoming orders quantity and orders on the other side...

            while(incoming->quantity > 0 && it != opposite_book.end()) {
                double best_price = it->first;
                Order* book_order = it->second; // Head of Linked List Using Linked List cuz works on pointers ..... faster than a Segment Tree...

                /* Check Price:
                    Buy : incoming price >= Best ask
                    sell : incoming price <= Best Bid
                */

                bool can_match = (incoming->side == Side :: BUY) ? (incoming->price >= best_price) : (incoming->price <= best_price);

                if(!can_match) break;

                double trade_qty = std::min(incoming->quantity, book_order->quantity); // trade execution......

                //trade logger
                std::cout << "[EXEC] " << (incoming->side == Side::BUY ? "BUY " : "SELL ") << trade_qty << " @ " << best_price << "\n";

                incoming->quantity -= trade_qty;
                book_order->quantity -= trade_qty;


                if(book_order -> quantity == 0){
                    //if book order is empty remove from list and move head to next order 
                    it->second = book_order->next;  //Moving Head to Pointer


                    if(it->second == nullptr){
                        it = opposite_book.erase(it);  // if linked lsit empty delete the price level from map,

                        continue; // continue the loop
                    }
                }
            }

            if(incoming->quantity > 0){
                insert_order(same_side_book, incoming); // if not fullfilled rest the order in book....
            }
        }

        template<typename BookMap>
        void insert_order(BookMap& book, Order* order){  
            auto it = book.find(order->price);
            if (it == book.end()){
                book[order->price] = order;     // new price level start the list
            } else {
                Order* current = it->second;
                while(current->next){          // append to the end of linked list
                    current = current->next;
                }
                current -> next = order;
            }
        }

    };
}

//main.cpp on src/engine for launching the engine... on static data sets.....