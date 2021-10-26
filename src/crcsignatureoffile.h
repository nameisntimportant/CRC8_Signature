#pragma once

#include "concurentqueue.h"
#include "crchasher.h"
#include "datafilewrapper.h"
#include "programmoptions.h"

#include <boost/asio/thread_pool.hpp>

#include <optional>

namespace Test
{
namespace CrcSignatureOfFileTestSuite
{
struct CleanupTest;
}
} // namespace Test

class CrcSignatureOfFile
{
public:
    explicit CrcSignatureOfFile(const Options& options);
    void readCalculateAndWrite();
    ~CrcSignatureOfFile();

private:
    static void cleanup(boost::asio::thread_pool& pool,
                        const std::string_view outputFileName,
                        std::optional<size_t> originalSizeOfOutputFile);

private:
    boost::asio::thread_pool pool_;

    size_t readTasksCnt_ = 0;
    Parallel::Queue<DataFrame> inputQueue_;
    Parallel::DataFileWrapper inputFile_;

    size_t crcCaclulationTasksCnt_ = 0;
    size_t blockSize_ = 0;
    Parallel::Crc8Wrapper crc8Hasher_;

    Parallel::Queue<DataFrame> outputQueue_;
    Parallel::DataFileWrapper outputFile_;
    std::string outputFileName_;
    std::optional<uintmax_t> originalSizeOfOutputFile_;

    bool success_ = false;

    friend Test::CrcSignatureOfFileTestSuite::CleanupTest;
};
