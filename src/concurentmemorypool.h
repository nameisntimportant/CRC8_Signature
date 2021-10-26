#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>

#include <boost/pool/pool_alloc.hpp>

namespace Parallel
{
class LazyMemoryPool
{
public:
    LazyMemoryPool() = default;

    void* allocate(size_t n);
    void deallocate(void* chunk);

private:
    mutable std::mutex mut_;
    std::once_flag poolInitialisationFlag_;
    std::unique_ptr<boost::pool<>> pool_;
    std::condition_variable poolCond_;
    size_t chunkSize_;
};
using LazyMemoryPoolPtr = std::shared_ptr<LazyMemoryPool>;
} // namespace Parallel
