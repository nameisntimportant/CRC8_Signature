#pragma once

#include <boost/range.hpp>

#include <atomic>
#include <memory>

using ConstDataIterator = const unsigned char*;
using DataRange = boost::iterator_range<unsigned char*>;
using ConstDataRange = boost::iterator_range<ConstDataIterator>;

template <typename T>
using SharedAtomic = std::shared_ptr<std::atomic<T>>;
