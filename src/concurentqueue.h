#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

namespace Parallel
{
template <typename T>
class Queue
{
    using milliseconds = std::chrono::milliseconds;
public:
    Queue(size_t maxSize = 0)
        : maxSize_(maxSize){};

    bool waitAndPop(T& value, const milliseconds timeToWait = milliseconds(0))
    {
        std::unique_lock<std::mutex> lk(mut_);
        if (!pushCond_.wait_for(lk, timeToWait, [this] { return !dataQueue_.empty(); }))
            return false;
        value = std::move(*dataQueue_.front());
        dataQueue_.pop();
        popCond_.notify_one();
        return true;
    }

    bool tryPop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut_);
        if (dataQueue_.empty())
            return false;
        value = std::move(*dataQueue_.front());
        dataQueue_.pop();
        popCond_.notify_one();
        return true;
    }

    void waitAndPush(T newValue)
    {
        std::shared_ptr<T> data(std::make_shared<T>(std::move(newValue)));
        std::unique_lock<std::mutex> lk(mut_);
        if (maxSize_ != 0)
            popCond_.wait(lk, [this] { return dataQueue_.size() < maxSize_; });
        dataQueue_.push(data);
        pushCond_.notify_one();
    }

private:
    mutable std::mutex mut_;
    const size_t maxSize_;
    std::queue<std::shared_ptr<T>> dataQueue_;
    std::condition_variable pushCond_;
    std::condition_variable popCond_;
};
} // namespace Parallel
