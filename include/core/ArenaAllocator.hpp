//Author : Movazed  ||  Added comments so we dont get lost We code from here.... 1st Module -> Arena Allocator -> LockFreeQueue
//Writing descriptive types so not getting lost and easier for other devs
#pragma once
#include <vector>
#include <cstddef>  
#include <cstdint>  
#include <new>      
#include <memory>   
#include <stdexcept>

namespace core {

class ArenaAllocator {
public: 
    explicit ArenaAllocator(size_t size_bytes) : offset_(0){
        // using explicit to prevent the compiler from doing unconventional conversions
        // reserving memory instantly
        // using vector<byte> for gauranteed contiguous memory use std:: ofcourse ;)

        buffer_.resize(size_bytes);
    }

    ArenaAllocator(const ArenaAllocator&) = delete; //safety : delete , arena owns it memory 
    //copying memory is disaster we delete to forbid it from copying

    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    // nodiscard is for allocate memory but stop it from catching pointer -> Compiler errors out no silent memory leaks

    template<typename T>
    [[nodiscard]] T* allocate(){
        const size_t size = sizeof(T);
        const size_t alignment = alignof(T);

        void* ptr  = static_cast<void*>(buffer_.data() + offset_); //get pointer to current free slot ....

        size_t space = buffer_.size() - offset_;    // calculate remaining space

        // below we can see optimizer is align this shifts ptr foward until it finds a memory address divisble by alignment
        // we are using this because misaligned data forces CPU to read 2 instead of 1 , we desire 1

        if(std::align(alignment, size, ptr, space)){
            size_t new_offset = static_cast<std::byte*>(ptr) - buffer_.data(); // the new offset after the alignment shift //"bump" the pointer forward

            offset_ = new_offset + size; // construct the object T in the memory we already have.. // no malloc , no kernal calls , 0 nanosecond latency , HOPE SOO.

            return new (ptr) T();
        }

        throw std::bad_alloc(); //resetting is free, we just move the integer index back to 0. // using noexcept since we want compiler no to fail, and enable further optimization
    }

    void reset() noexcept{
        offset_ = 0;
    }

    [[nodiscard]] size_t used_bytes() const noexcept {
        return offset_;
    }


private:
    std::vector<std::byte> buffer_;
    size_t offset_;

    };
}


//we messed up at first attempt......