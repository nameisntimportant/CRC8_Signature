#include "zerofilledmemory.h"

#include <cstring>

ZeroFilledMemory::ZeroFilledMemory(size_t n, Parallel::LazyMemoryPoolPtr memoryPool)
{
    memory_ = allocate(n, memoryPool.get());
    memset(memory_, 0, n);
    memoryPool_ = memoryPool;
    capacity_ = n;
}

ZeroFilledMemory::ZeroFilledMemory(ZeroFilledMemory&& other) noexcept
{
    swap(other);
}

void ZeroFilledMemory::swap(ZeroFilledMemory& other) noexcept
{
    std::swap(memory_, other.memory_);
    std::swap(memoryPool_, other.memoryPool_);
    std::swap(capacity_, other.capacity_);
}

ZeroFilledMemory& ZeroFilledMemory::operator=(ZeroFilledMemory&& other) noexcept
{
    swap(other);
    return *this;
}

const unsigned char* ZeroFilledMemory::begin() const noexcept
{
    return memory_;
}

unsigned char* ZeroFilledMemory::begin() noexcept
{
    return memory_;
}

size_t ZeroFilledMemory::capacity() const noexcept
{
    return capacity_;
}

Parallel::LazyMemoryPoolPtr ZeroFilledMemory::memoryPool() const
{
    return memoryPool_;
}

unsigned char* ZeroFilledMemory::allocate(size_t n, Parallel::LazyMemoryPool* memoryPool)
{
    auto* memory = memoryPool ? memoryPool->allocate(n) : (operator new(n));
    return static_cast<unsigned char*>(memory);
}

void ZeroFilledMemory::deallocate(unsigned char* buf, Parallel::LazyMemoryPool* memoryPool)
{
    memoryPool ? memoryPool->deallocate(buf) : operator delete(buf);
}

ZeroFilledMemory::~ZeroFilledMemory()
{
    deallocate(memory_, memoryPool_.get());
}
