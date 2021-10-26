#pragma once

#include <assert.h>
#include <atomic>
#include <memory>

template <class T, class U>
auto makeSharedAtomic(U&& args)
{
    return std::make_shared<std::atomic<T>>(std::forward<U>(args));
}

inline size_t ceilDevision(size_t divisible, size_t divisor)
{
    assert(divisor != 0);
    auto result = divisible / divisor;
    return (divisible % divisor != 0) ? ++result : result;
}
