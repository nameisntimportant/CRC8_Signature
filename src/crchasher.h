#pragma once

#include "concurentqueue.h"
#include "dataframe.h"
#include "defs.h"

#include <boost/asio/thread_pool.hpp>

#include <future>

using Crc8ResultType = unsigned char;

// NOTE: CRC-8-Dallas/Maxim
Crc8ResultType crc8(ConstDataRange range);
DataFrame calculateCrc8OfFrame(const DataFrame& inFrame, Parallel::LazyMemoryPoolPtr memoryPool);

namespace Parallel
{
class Crc8Wrapper
{
public:
    struct CalculateForWholeQueueParams
    {
        Queue<DataFrame>& src;
        Queue<DataFrame>& dest;
        const SharedAtomic<bool> hasProducerFinished;
        size_t tasksCount;
        boost::asio::thread_pool& pool;
    };

public:
    void calculateForWholeQueue(CalculateForWholeQueueParams params);
    void joinAndRethrowExceptions();

private:
    std::vector<std::future<void>> futures_;
};
} // namespace Parallel
