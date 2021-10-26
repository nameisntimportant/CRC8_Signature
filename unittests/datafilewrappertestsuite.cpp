#include <boost/test/unit_test.hpp>

#include <math.h>

#include "datafilewrapper.h"
#include "defs.h"
#include "memorysizeliterals.h"
#include "testdefs.h"
#include "testtools.h"
#include "utils.h"

namespace fs = std::filesystem;
using iob = std::ios_base;

bool operator==(const DataFrameConfig& lhs, const DataFrameConfig& rhs)
{
    return lhs.firstBlockIdx == rhs.firstBlockIdx && lhs.blockSize == rhs.blockSize &&
           lhs.blocksCount == rhs.blocksCount;
}

bool operator!=(const DataFrameConfig& lhs, const DataFrameConfig& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& stream, const DataFrameConfig& frame)
{
    return stream << "thirst block index " << frame.firstBlockIdx
                  << "block size: " << frame.blockSize << " blocksCount: " << frame.blocksCount;
}

namespace Test
{
using namespace Parallel;

namespace
{
std::vector<unsigned char> getAllFramesDataAsVector(Queue<DataFrame>& frames)
{
    std::vector<unsigned char> result;
    DataFrame frame;
    while (frames.tryPop(frame))
    {
        const auto dataBeginShift = frame.firstBlockIndex() * frame.blockSize();
        const auto newTotalSize = dataBeginShift + frame.totalSizeOfAllBlocks();
        result.resize(std::max(result.size(), newTotalSize));
        copy(frame.begin(), frame.end(), next(result.begin(), dataBeginShift));
    }
    return result;
}

void testReadAllAsDataFrames(const size_t dataBlockSize, const size_t threadCount)
{
    // NOTE: file reading is checked in datafiletestsuite. Here we only check that parallel readed
    // data is equal to serial one
    assert(fs::exists(PermanentTestFileName));
    const auto fileSize = fs::file_size(PermanentTestFileName);
    const size_t blocksInFile = ceilDevision(fileSize, dataBlockSize);

    boost::asio::thread_pool pool;
    Queue<DataFrame> parallelResult;

    DataFileWrapper reader(PermanentTestFileName, iob::in | iob::binary);
    reader.readAllAsDataFrames({.dest = parallelResult,
                                .dataBlockSize = dataBlockSize,
                                .tasksCount = threadCount,
                                .pool = pool});
    reader.joinAndRethrowExceptions();
    auto parallelResultAsVector = getAllFramesDataAsVector(parallelResult);

    const DataFrameConfig readAllFileInOneFrameConfig{
        .firstBlockIdx = 0, .blockSize = dataBlockSize, .blocksCount = blocksInFile};
    DataFile serialReader(PermanentTestFileName, iob::in | iob::binary);
    auto serialResult = serialReader.readDataBlocksAsFrame(readAllFileInOneFrameConfig);

    BOOST_CHECK_EQUAL_COLLECTIONS(parallelResultAsVector.begin(),
                                  parallelResultAsVector.end(),
                                  serialResult.begin(),
                                  serialResult.end());
}
} // namespace

BOOST_AUTO_TEST_SUITE(DataFileWrapperTestSuite)
BOOST_AUTO_TEST_CASE(DistributeDataBlocksBetweenFileSegmentsTest)
{
    // NOTE: We test all those cases in one test case in order to not declare plenty of
    // BOOST_AUTO_TEST_CASE as friends of DataFileWrapper
    auto memoryPool = std::make_shared<LazyMemoryPool>();
    {
        const auto fileSize = 0;
        const auto blockSize = 10;
        const auto res = DataFileWrapper::makeConfigs(fileSize, blockSize, memoryPool);
        const auto exp = std::vector<DataFrameConfig>{};
        BOOST_CHECK_EQUAL_COLLECTIONS(res->begin(), res->end(), exp.begin(), exp.end());
    }
    {
        const auto fileSize = 1;
        const auto blockSize = 10;
        const auto res = DataFileWrapper::makeConfigs(fileSize, blockSize, memoryPool);
        const auto exp = std::vector<DataFrameConfig>{{.firstBlockIdx = 0,
                                                       .blockSize = blockSize,
                                                       .blocksCount = 1,
                                                       .memoryPool = memoryPool}};
        BOOST_CHECK_EQUAL_COLLECTIONS(res->begin(), res->end(), exp.begin(), exp.end());
    }
    {
        const auto fileSize = 10;
        const auto blockSize = 3;
        const auto res = DataFileWrapper::makeConfigs(fileSize, blockSize, memoryPool);
        const auto exp = std::vector<DataFrameConfig>{{.firstBlockIdx = 0,
                                                       .blockSize = blockSize,
                                                       .blocksCount = 4,
                                                       .memoryPool = memoryPool}};
    }
    {
        const auto fileSize = MB;
        const auto blockSize = 3;
        auto res = DataFileWrapper::makeConfigs(fileSize, blockSize, memoryPool);
        const size_t expectedBlocksCount = ceilDevision(fileSize, blockSize);
        const auto exp = std::vector<DataFrameConfig>{{.firstBlockIdx = 0,
                                                       .blockSize = blockSize,
                                                       .blocksCount = expectedBlocksCount,
                                                       .memoryPool = memoryPool}};
        BOOST_CHECK_EQUAL_COLLECTIONS(res->begin(), res->end(), exp.begin(), exp.end());
    }
    {
        const auto fileSize = 3 * MB;
        const auto blockSize = 2;
        const auto res = DataFileWrapper::makeConfigs(fileSize, blockSize, memoryPool);
        const size_t expectedBlocksCountPerFrame = ceilDevision(MB, blockSize);
        const auto exp =
            std::vector<DataFrameConfig>{{.firstBlockIdx = 0,
                                          .blockSize = blockSize,
                                          .blocksCount = expectedBlocksCountPerFrame,
                                          .memoryPool = memoryPool},
                                         {.firstBlockIdx = expectedBlocksCountPerFrame,
                                          .blockSize = blockSize,
                                          .blocksCount = expectedBlocksCountPerFrame,
                                          .memoryPool = memoryPool},
                                         {.firstBlockIdx = expectedBlocksCountPerFrame * 2,
                                          .blockSize = blockSize,
                                          .blocksCount = expectedBlocksCountPerFrame,
                                          .memoryPool = memoryPool}};
        BOOST_CHECK_EQUAL_COLLECTIONS(res->begin(), res->end(), exp.begin(), exp.end());
    }
    {
        const auto blockSize = 2;
        const auto fileSize = MB + 453;
        const auto res = DataFileWrapper::makeConfigs(fileSize, blockSize, memoryPool);
        const size_t expectedBlocksCountPerFrame = ceilDevision(MB, blockSize);
        const auto exp =
            std::vector<DataFrameConfig>{{.firstBlockIdx = 0,
                                          .blockSize = blockSize,
                                          .blocksCount = expectedBlocksCountPerFrame,
                                          .memoryPool = memoryPool},
                                         {.firstBlockIdx = expectedBlocksCountPerFrame,
                                          .blockSize = blockSize,
                                          .blocksCount = expectedBlocksCountPerFrame,
                                          .memoryPool = memoryPool}};
        BOOST_CHECK_EQUAL_COLLECTIONS(res->begin(), res->end(), exp.begin(), exp.end());
    }
}

struct ReadingTaskFixture
{
    DataFrameConfig thirstConfig;
    DataFrameConfig exceptionLeadingConfig;
    DataFrameConfig lastConfig;
    Queue<DataFrameConfig> configs;

    DataFrame expectedFirstFrameData;
    AutoFileRemover remover;

    Queue<DataFrame> result;

    ReadingTaskFixture()
        : remover(createAutoRemovableFileWithContent(TempTestFileName,
                                                     {{0x00, 0xAA, 0xCC, 0x23, 0x4F}}))
    {
        const size_t tooBigBlockSize = pow(1024, 3) * 10;
        thirstConfig = {.firstBlockIdx = 1, .blockSize = 2, .blocksCount = 1};
        exceptionLeadingConfig = {
            .firstBlockIdx = 14, .blockSize = tooBigBlockSize, .blocksCount = 4};
        lastConfig = {.firstBlockIdx = 3, .blockSize = 2, .blocksCount = 4};

        configs.waitAndPush(thirstConfig);
        configs.waitAndPush(exceptionLeadingConfig);
        configs.waitAndPush(lastConfig);

        expectedFirstFrameData = createDataFrameWithData(1, {{0xCC, 0x23}});
    }
};

BOOST_AUTO_TEST_CASE(ReadAllAsDataFramesTest)
{
    testReadAllAsDataFrames(12, 4);
    testReadAllAsDataFrames(KB * 14, 11);
    testReadAllAsDataFrames(MB, 7);
    testReadAllAsDataFrames(MB * 97, 3);
}

struct WriteAllDataFramesFixture
{
    Queue<DataFrame> input;
    boost::asio::thread_pool pool = boost::asio::thread_pool(3);
    SharedAtomic<bool> finishFlag = makeSharedAtomic<bool>(true);
};

BOOST_FIXTURE_TEST_CASE(WriteAllDataFramesEmptyQueueTest, WriteAllDataFramesFixture)
{
    auto fileRemover =
        createAutoRemovableFileWithContent(TempTestFileName, {{0x01, 0x02, 0x03, 0x04}});

    DataFileWrapper writer(TempTestFileName, (iob::out | iob::in | iob::binary));
    writer.writeAllDataFrames(
        {.src = input, .hasProducerFinished = finishFlag, .pool = pool, .writingPosShift = 2});
    writer.joinAndRethrowExceptions();

    auto result = readWholeFile(TempTestFileName);
    std::vector<unsigned char> expected = {0x01, 0x02, 0x03, 0x04};
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(WriteAllDataFramesNonEmptyQueueTest, WriteAllDataFramesFixture)
{
    auto fileRemover =
        createAutoRemovableFileWithContent(TempTestFileName, {{0x01, 0x02, 0x03, 0x04}});
    input.waitAndPush(createDataFrameWithData(2, {{0x22, 0x33}}));
    input.waitAndPush(createDataFrameWithData(0, {{0x01, 0x23}}));
    input.waitAndPush(createDataFrameWithData(1, {{0xF0, 0x3F}}));

    DataFileWrapper writer(TempTestFileName, (iob::out | iob::in | iob::binary));
    writer.writeAllDataFrames(
        {.src = input, .hasProducerFinished = finishFlag, .pool = pool, .writingPosShift = 2});
    writer.joinAndRethrowExceptions();

    auto result = readWholeFile(TempTestFileName);
    std::vector<unsigned char> expected = {0x01, 0x02, 0x01, 0x23, 0xF0, 0x3F, 0x22, 0x33};
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_CASE(WriteAllDataFramesToNonExistingFileTest, WriteAllDataFramesFixture)
{
    assert(!fs::exists(TempTestFileName));
    AutoFileRemover remover(TempTestFileName);
    input.waitAndPush(createDataFrameWithData(2, {{0x22, 0x33}}));
    input.waitAndPush(createDataFrameWithData(0, {{0x01, 0x23}}));
    input.waitAndPush(createDataFrameWithData(1, {{0xF0, 0x3F}}));

    DataFileWrapper writer(TempTestFileName, (iob::out | iob::binary));
    writer.writeAllDataFrames(
        {.src = input, .hasProducerFinished = finishFlag, .pool = pool, .writingPosShift = 3});
    writer.joinAndRethrowExceptions();

    auto result = readWholeFile(TempTestFileName);
    std::vector<unsigned char> expected = {0x00, 0x00, 0x00, 0x01, 0x023, 0xF0, 0x3F, 0x22, 0x33};
    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace Test
