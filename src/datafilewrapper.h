#pragma once

#include "concurentmemorypool.h"
#include "concurentqueue.h"
#include "datafile.h"

#include <boost/asio/thread_pool.hpp>

#include <future>

namespace Test
{
namespace DataFileWrapperTestSuite
{
struct DistributeDataBlocksBetweenFileSegmentsTest;
struct BadAllocInNotTheLastReadingTaskTest;
struct BadAllocInTheLastReadingTaskTest;
} // namespace DataFileWrapperTestSuite
} // namespace Test

namespace Parallel
{
class DataFileWrapper
{
public:
    struct ReadAllAsDataFramesParams
    {
        Queue<DataFrame>& dest;
        size_t dataBlockSize;
        size_t tasksCount;
        boost::asio::thread_pool& pool;
    };

    struct WriteAllDataFramesParams
    {
        Queue<DataFrame>& src;
        SharedAtomic<bool> hasProducerFinished;
        boost::asio::thread_pool& pool;
        uintmax_t writingPosShift;
    };

public:
    DataFileWrapper(const std::string& path, std::ios_base::openmode mode);

    void readAllAsDataFrames(ReadAllAsDataFramesParams params);
    void writeAllDataFrames(WriteAllDataFramesParams params);

    void joinAndRethrowExceptions();

private:
    static DataFrameConfigsPtr makeConfigs(uintmax_t fileSize,
                                           size_t dataBlockSize,
                                           LazyMemoryPoolPtr memoryPool);

private:
    std::string path_;
    std::ios_base::openmode mode_;

    // NOTE: We use std::future::get() to join reading threads and get exceptions if there are some
    std::vector<std::future<void>> futures_;

    friend Test::DataFileWrapperTestSuite::DistributeDataBlocksBetweenFileSegmentsTest;
    friend Test::DataFileWrapperTestSuite::BadAllocInNotTheLastReadingTaskTest;
    friend Test::DataFileWrapperTestSuite::BadAllocInTheLastReadingTaskTest;
};
} // namespace Parallel
