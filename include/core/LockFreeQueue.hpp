//Author : Movazed  ||  Added comments so we dont get lost We code from here.... 1st Module -> Arena Allocator -> LockFreeQueue
//Writing descriptive types so not getting lost and easier for other devs

#pragma once
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <new>

namespace core {
    //checking if compiler knows the cpu cache line size, if not guess 64 byte.
#ifdef __cpp_lib_hardware_interference_size
    constexpr size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
#else
    constexpr size_t CACHE_LINE_SIZE = 64;
#endif

template<typename T, size_t Size>
class LockFreeQueue{
    // we are only going to allow simple trivial types, complex type with internal pointers are unsafe here.
    static_assert(std::is_trivially_copyable_v<T>, "Queue T must be trivial and could be copied");
    static_assert((Size & (Size - 1)) == 0, "Queue Size should be power of 2 such as 2, 4, 8,..."); //force size to be a power 2 , (size & (size - 1)) == 0 is a binary trick to check for powers of 2

    struct Slot{
        T value;    //the actual buffer
    };
    Slot buffer_[Size];

    // Alignment comes in play here we force, head and tail to live far apart in memory this stops producer and consumer thread to , clash over same RAM address.
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> head_{0}; // consumer index
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> tail_{0}; // producer index

public:
    //no discard forces to check bool result true = success, false = full
    [[nodiscard]] bool push(const T& val) noexcept {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);

        //using bitwise tech to increase the speed -> we dont use (tail + 1) % Size, we do (tail + 1) & (Size - 1) -> This is 20 x faster than normal division
        const size_t next_tail = (current_tail + 1) & (Size - 1);


        if(next_tail == head_.load(std::memory_order_acquire)){ // check if full ensures head updates...
                return false;
        }
            buffer_[current_tail].value = val;

            //release makes the write variable to other thread immediately
            tail_.store(next_tail, std::memory_order_release);
            return true;

        }
        [[nodiscard]] bool pop(T& val) noexcept {
            // changed 'current_tail' to 'current_head'
            const size_t current_head = head_.load(std::memory_order_relaxed);
            //check for empty

            if (current_head == tail_.load(std::memory_order_acquire)) {
                        return false; 
                    }

            val = buffer_[current_head].value;

            const size_t next_head = (current_head + 1) & (Size - 1);
            head_.store(next_head, std::memory_order_release);
            return true;  //shfited too next head on linked list 
        }
    };
}

//Your biggest question might be how to get this running right for that we will build the Engine for that so move to include/engine/OrderBook.hpp for next codes cont..