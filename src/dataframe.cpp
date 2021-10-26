#include <assert.h>

#include "dataframe.h"

DataFrame::DataFrame(DataFrameConfig config)
    : firstBlockIdx_(config.firstBlockIdx)
    , blocksCount_(config.blocksCount)
    , blockSize_(config.blockSize)
    , data_(config.blockSize * config.blocksCount, std::move(config.memoryPool))
{
    assert(blockSize_);
}

DataFrame::DataFrame(const DataFrame& other)
    : firstBlockIdx_(other.firstBlockIdx_)
    , blocksCount_(other.blocksCount_)
    , blockSize_(other.blockSize_)
    , data_(other.data_.capacity(), other.data_.memoryPool())
{
    std::uninitialized_copy_n(other.data_.begin(), other.totalSizeOfAllBlocks(), data_.begin());
}

DataFrame::DataFrame(DataFrame&& other) noexcept
{
    swap(other);
}

DataFrame& DataFrame::operator=(const DataFrame& other)
{
    DataFrame tmp(other);
    swap(tmp);
    return *this;
}

DataFrame& DataFrame::operator=(DataFrame&& other) noexcept
{
    swap(other);
    return *this;
}

void DataFrame::swap(DataFrame& other) noexcept
{
    std::swap(firstBlockIdx_, other.firstBlockIdx_);
    std::swap(blocksCount_, other.blocksCount_);
    std::swap(blockSize_, other.blockSize_);
    data_.swap(other.data_);
}

char* DataFrame::data() noexcept
{
    return reinterpret_cast<char*>(data_.begin());
}

const char* DataFrame::data() const noexcept
{
    return reinterpret_cast<const char*>(data_.begin());
}

DataRange DataFrame::blockAsRange(size_t blockIndex)
{
    const auto begin = std::next(data_.begin(), blockIndex * blockSize_);
    const auto end = std::next(begin, blockSize_);
    return {begin, end};
}

ConstDataRange DataFrame::blockAsRange(size_t blockIndex) const
{
    const auto begin = std::next(data_.begin(), blockIndex * blockSize_);
    const auto end = std::next(begin, blockSize_);
    return {begin, end};
}

DataFrame::iterator DataFrame::begin() noexcept
{
    return data_.begin();
}

DataFrame::const_iterator DataFrame::cbegin() const noexcept
{
    return data_.begin();
}

DataFrame::iterator DataFrame::end() noexcept
{
    return data_.begin() + totalSizeOfAllBlocks();
}

DataFrame::const_iterator DataFrame::cend() const noexcept
{
    return data_.begin() + totalSizeOfAllBlocks();
}

void DataFrame::setBlocksCount(size_t blocksCount)
{
    if (blocksCount * blockSize_ > data_.capacity())
        throw std::out_of_range("The size of the new number of blocks is larger than the capacity");
    blocksCount_ = blocksCount;
}

size_t DataFrame::blocksCount() const noexcept
{
    return blocksCount_;
}

size_t DataFrame::totalSizeOfAllBlocks() const noexcept
{
    return blocksCount_ * blockSize_;
}

uintmax_t DataFrame::firstBlockIndex() const noexcept
{
    return firstBlockIdx_;
}

size_t DataFrame::blockSize() const noexcept
{
    return blockSize_;
}

size_t DataFrame::capacity() const noexcept
{
    return data_.capacity();
}
