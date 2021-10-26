#pragma once

#include "concurentmemorypool.h"

class ZeroFilledMemory
{
public:
    ZeroFilledMemory() = default;
    ZeroFilledMemory(const ZeroFilledMemory&) = delete;

    ZeroFilledMemory(size_t n, Parallel::LazyMemoryPoolPtr memoryPool = nullptr);
    ZeroFilledMemory(ZeroFilledMemory&& other) noexcept;

    void swap(ZeroFilledMemory& other) noexcept;

    ZeroFilledMemory& operator=(const ZeroFilledMemory&) = delete;
    ZeroFilledMemory& operator=(ZeroFilledMemory&& other) noexcept;

    const unsigned char* begin() const noexcept;
    unsigned char* begin() noexcept;

    size_t capacity() const noexcept;
    Parallel::LazyMemoryPoolPtr memoryPool() const;

    ~ZeroFilledMemory();

private:
    static unsigned char* allocate(size_t n, Parallel::LazyMemoryPool* memoryPool);
    void deallocate(unsigned char* buf, Parallel::LazyMemoryPool* memoryPool);

private:
    unsigned char* memory_ = nullptr;
    Parallel::LazyMemoryPoolPtr memoryPool_;
    size_t capacity_ = 0;
};
