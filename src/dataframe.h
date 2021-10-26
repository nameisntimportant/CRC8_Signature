#pragma once

#include "zerofilledmemory.h"
#include "defs.h"

#include <boost/range/sub_range.hpp>

#include <memory>

struct DataFrameConfig
{
    uintmax_t firstBlockIdx = 0;
    size_t blockSize = 1;
    size_t blocksCount = 0;
    Parallel::LazyMemoryPoolPtr memoryPool = nullptr;
};
using DataFrameConfigs = std::vector<DataFrameConfig>;
using DataFrameConfigsPtr = std::shared_ptr<DataFrameConfigs>;

class DataFrame
{
public:
    explicit DataFrame(DataFrameConfig config = {});
    DataFrame(const DataFrame& other);
    DataFrame(DataFrame&& other) noexcept;

    DataFrame& operator=(const DataFrame& other);
    DataFrame& operator=(DataFrame&& other) noexcept;

    void swap(DataFrame& other) noexcept;

    [[nodiscard]] char* data() noexcept;
    [[nodiscard]] const char* data() const noexcept;

    using iterator = unsigned char*;
    using const_iterator = const unsigned char*;

    [[nodiscard]] DataRange blockAsRange(size_t blockIndex);
    [[nodiscard]] ConstDataRange blockAsRange(size_t blockIndex) const;

    [[nodiscard]] iterator begin() noexcept;
    [[nodiscard]] const_iterator cbegin() const noexcept;

    [[nodiscard]] iterator end() noexcept;
    [[nodiscard]] const_iterator cend() const noexcept;

    // NOTE: DataFrame is designed to be used with a memory pool with fixed memory chunks size.
    // So capacity is fixed and setBlocksCount() doesn't change actual space occupied in memory.
    [[nodiscard]] size_t blocksCount() const noexcept;
    void setBlocksCount(size_t blocksCount);

    [[nodiscard]] size_t blockSize() const noexcept;
    [[nodiscard]] size_t totalSizeOfAllBlocks() const noexcept;
    [[nodiscard]] size_t capacity() const noexcept;
    [[nodiscard]] uintmax_t firstBlockIndex() const noexcept;

private:
    uintmax_t firstBlockIdx_;
    size_t blocksCount_;
    size_t blockSize_;
    ZeroFilledMemory data_;
};
