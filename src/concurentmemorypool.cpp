#include "concurentmemorypool.h"

namespace
{
constexpr auto AcceptableQuantityOfAlloacationsFailing = 10'000;
}

namespace Parallel
{
void* LazyMemoryPool::allocate(size_t n)
{
    std::call_once(poolInitialisationFlag_, [this, n]() {
        pool_ = std::make_unique<boost::pool<>>(n);
        chunkSize_ = n;
    });

    assert(chunkSize_ == n);
    void* chunk = nullptr;
    for (size_t i = 0; i < AcceptableQuantityOfAlloacationsFailing; i++)
    {
        std::lock_guard<std::mutex> lk(mut_);
        chunk = pool_->malloc();
        if (chunk)
            break;
    }

    if (!chunk)
        throw std::bad_alloc();

    return chunk;
}

void LazyMemoryPool::deallocate(void* chunk)
{
    std::lock_guard<std::mutex> lk(mut_);
    pool_->free(chunk);
}
} // namespace Parallel
