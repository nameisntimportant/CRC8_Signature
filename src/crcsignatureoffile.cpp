#include "crcsignatureoffile.h"
#include "utils.h"

#include <filesystem>
#include <iostream>

#include <boost/thread/thread.hpp>

namespace fs = std::filesystem;
using iob = std::ios_base;

namespace
{
size_t getThreadCnt()
{
    // NOTE: We need at least one thread per reading, calculating and writing
    if (boost::thread::hardware_concurrency() == 0)
        return 3;
    else
        return std::max(3u, boost::thread::hardware_concurrency() - 1);
}

size_t getMaxQueueSize(size_t dataBlockSize, size_t crcHasherResultSize, size_t maxRamSize)
{
    // NOTE: We allocate RAM in such a way that the input and output queues have the same maximum
    // number of elements
    const auto maxQueueSize = (maxRamSize / (dataBlockSize + crcHasherResultSize));
    if (maxQueueSize == 0)
    {
        throw std::invalid_argument(
            "Max RAM size is too small to proceed data blocks with such a size. "
            "Please, either reduce data block size, either increase max RAM size. Please run the "
            "program with --help parametr for more information");
    }
    return maxQueueSize;
}

std::ios_base::openmode getOpenModeForOutputFile(const std::string_view& path)
{
    const auto inOutMode = fs::exists(path) ? (iob::in | iob::out) : iob::out;
    return iob::binary | inOutMode;
}
} // namespace

CrcSignatureOfFile::CrcSignatureOfFile(const Options& options)
    : pool_(getThreadCnt())
    , readTasksCnt_(options.isSSD ? ceilDevision(getThreadCnt(), 4) : 1)
    , inputQueue_(getMaxQueueSize(options.blockSize, sizeof(Crc8ResultType), options.maxRamSize))
    , inputFile_(options.inputFile, (iob::binary | iob::in))
    , crcCaclulationTasksCnt_(ceilDevision((getThreadCnt() * 3), 4))
    , blockSize_(options.blockSize)
    , outputQueue_(getMaxQueueSize(options.blockSize, sizeof(Crc8ResultType), options.maxRamSize))
    , outputFile_(options.outputFile, getOpenModeForOutputFile(options.outputFile))
    , outputFileName_(options.outputFile)
    , originalSizeOfOutputFile_(fs::exists(options.outputFile)
                                    ? std::optional(fs::file_size(options.outputFile))
                                    : std::nullopt)
{
    if (blockSize_ == 0)
        throw std::logic_error("the block size cannot be zero");

    // NOTE: We need reading tasks count plus writing tasks count is less than getThreadCnt()
    // because otherwise we will not be able to post any crc calculation tasks
    const auto writingTasksCnt = 1;
    assert(readTasksCnt_ + writingTasksCnt < getThreadCnt());
};

void CrcSignatureOfFile::readCalculateAndWrite()
{
    success_ = false;

    auto isReadingFinished = makeSharedAtomic<bool>(false);
    inputFile_.readAllAsDataFrames({.dest = inputQueue_,
                                    .dataBlockSize = blockSize_,
                                    .tasksCount = readTasksCnt_,
                                    .pool = pool_});

    // NOTE: We post writing tasks before calculating tasks to avoid situations when we fill whole
    // the pull with reading and calculating tasks and the writing task doesn't execute until any of
    // the reading or calculating tasks are finished. That situation might lead to a deadlock.
    auto isCrcCalculationFinished = makeSharedAtomic<bool>(false);
    outputFile_.writeAllDataFrames({.src = outputQueue_,
                                    .hasProducerFinished = isCrcCalculationFinished,
                                    .pool = pool_,
                                    .writingPosShift = originalSizeOfOutputFile_.value_or(0)});

    crc8Hasher_.calculateForWholeQueue({.src = inputQueue_,
                                        .dest = outputQueue_,
                                        .hasProducerFinished = isReadingFinished,
                                        .tasksCount = crcCaclulationTasksCnt_,
                                        .pool = pool_});

    try
    {
        inputFile_.joinAndRethrowExceptions();
    }
    catch (std::fstream::failure& e)
    {
        throw std::fstream::failure(
            "Error during working with input file: " + std::string(e.what()), e.code());
    }
    isReadingFinished->store(true);

    crc8Hasher_.joinAndRethrowExceptions();
    isCrcCalculationFinished->store(true);

    try
    {
        outputFile_.joinAndRethrowExceptions();
    }
    catch (std::fstream::failure& e)
    {
        throw std::fstream::failure(
            "Error during working with output file: " + std::string(e.what()), e.code());
    }
    success_ = true;
}

void CrcSignatureOfFile::cleanup(boost::asio::thread_pool& pool,
                                 const std::string_view outputFileName,
                                 const std::optional<uintmax_t> originalSizeOfOutputFile)
{
    pool.stop();
    if (originalSizeOfOutputFile)
        fs::resize_file(outputFileName, *originalSizeOfOutputFile);
    else
        fs::remove(outputFileName);
}

CrcSignatureOfFile::~CrcSignatureOfFile()
{
    if (success_)
        return;

    try
    {
        cleanup(pool_, outputFileName_, originalSizeOfOutputFile_);
    }
    catch (const fs::filesystem_error& err)
    {
        std::cout << "can't restore original content of " << outputFileName_
                  << "(or remove it if it didn't exist):\n \"" << err.what();
    }
    catch (...)
    {
        std::cout << "can't restore original content of " << outputFileName_
                  << "(or remove it if it didn't exist)";
    }
}
