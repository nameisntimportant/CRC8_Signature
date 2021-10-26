#include "datafilewrapper.h"
#include "memorysizeliterals.h"
#include "utils.h"

#include <boost/asio/post.hpp>
#include <boost/pool/pool_alloc.hpp>

#include <filesystem>

namespace Parallel
{
namespace
{
constexpr size_t OptimalDataFrameSize = MB;
} // namespace

DataFileWrapper::DataFileWrapper(const std::string& path, const std::ios_base::openmode mode)
    : path_(path)
    , mode_(mode){};

DataFrameConfigsPtr DataFileWrapper::makeConfigs(const uintmax_t fileSize,
                                                 const size_t dataBlockSize,
                                                 LazyMemoryPoolPtr memoryPool)
{
    assert(dataBlockSize != 0);
    if (fileSize == 0)
        return std::make_shared<DataFrameConfigs>();

    const size_t dataBlocksInFile = ceilDevision(fileSize, dataBlockSize);
    size_t dataBlocksInFrame = ceilDevision(OptimalDataFrameSize, dataBlockSize);
    dataBlocksInFrame = std::min(dataBlocksInFrame, dataBlocksInFile);
    const auto dataFramesInFile = ceilDevision(dataBlocksInFile, dataBlocksInFrame);

    auto result = std::make_shared<DataFrameConfigs>();
    result->reserve(dataFramesInFile);
    for (size_t i = 0; i < dataFramesInFile; i++)
    {
        result->push_back(DataFrameConfig{.firstBlockIdx = i * dataBlocksInFrame,
                                          .blockSize = dataBlockSize,
                                          .blocksCount = dataBlocksInFrame,
                                          .memoryPool = memoryPool});
    }
    return result;
}

void DataFileWrapper::readAllAsDataFrames(ReadAllAsDataFramesParams prms)
{
    assert(futures_.size() == 0 && prms.tasksCount != 0);

    const auto configs = makeConfigs(
        std::filesystem::file_size(path_), prms.dataBlockSize, std::make_shared<LazyMemoryPool>());
    auto currentConfigIndex = makeSharedAtomic<size_t>(0);

    for (size_t i = 0; i < prms.tasksCount; i++)
    {
        std::packaged_task<void()> task([=]() {
            DataFile file(path_, mode_);
            while (true)
            {
                size_t configIdx = (*currentConfigIndex)++;
                if (configIdx >= configs->size())
                    break;
                auto frame = file.readDataBlocksAsFrame(std::move(configs->at(configIdx)));
                prms.dest.waitAndPush(std::move(frame));
            }
        });
        futures_.push_back(task.get_future());
        post(prms.pool, std::move(task));
    }
}

void DataFileWrapper::writeAllDataFrames(WriteAllDataFramesParams prms)
{
    assert(futures_.size() == 0);

    std::packaged_task<void()> writingTask([&, prms]() {
        DataFile file(path_, mode_);

        DataFrame frame;
        while (!prms.hasProducerFinished->load())
        {
            while (prms.src.waitAndPop(frame, std::chrono::milliseconds(100)))
                file.writeDataFrame(frame, prms.writingPosShift);
        }
        while (prms.src.tryPop(frame))
            file.writeDataFrame(frame, prms.writingPosShift);
    });
    futures_.push_back(writingTask.get_future());
    post(prms.pool, std::move(writingTask));
}

void DataFileWrapper::joinAndRethrowExceptions()
{
    for (auto& future : futures_)
        future.get();
    futures_.clear();
}

} // namespace Parallel
