#include "crchasher.h"
#include "testtools.h"
#include "utils.h"

#include <boost/test/unit_test.hpp>

#include <set>

namespace Test
{
namespace
{
std::vector<DataFrame> getDefaultCrcInputData()
{
    std::vector<DataFrame> dest;
    dest.push_back(createDataFrameWithData(10, {{0x7B}, {0x32}, {0x00}, {0x0C}}));
    dest.push_back(createDataFrameWithData(15, {{0x7C}}));
    dest.push_back(createDataFrameWithData(0, {{0x7B, 0x47}}));
    dest.push_back(createDataFrameWithData(
        3, {{0x7B, 0x00, 0x43}, {0x32, 0x7B, 0x00}, {0x02, 0x70, 0x10}, {0x32, 0x7B, 0x00}}));
    dest.push_back(
        createDataFrameWithData(50, {{0x7B}, {0x32}, {0x00}, {0x0C}, {0x12}, {0x23}, {0x03}}));
    return dest;
}

std::vector<DataFrame> getDefaultExpectedCrcOutputData()
{
    std::vector<DataFrame> result;
    result.push_back(createDataFrameWithData(10, {{0x12}, {0xA7}, {0x00}, {0x7D}}));
    result.push_back(createDataFrameWithData(15, {{0x85}}));
    result.push_back(createDataFrameWithData(0, {{0x8B}}));
    result.push_back(createDataFrameWithData(3, {{0xD9}, {0x70}, {0xF4}, {0x70}}));
    result.push_back(
        createDataFrameWithData(50, {{0x12}, {0xA7}, {0x00}, {0x7D}, {0x21}, {0xD5}, {0x53}}));
    return result;
}

void testCalculateForWholeQueue(std::vector<DataFrame> inputData,
                                std::vector<DataFrame> outputData,
                                size_t threadCnt)
{
    Parallel::Queue<DataFrame> outputQueue;
    auto hasProducerFinished = makeSharedAtomic<bool>(false);
    boost::asio::thread_pool pool(threadCnt);

    Parallel::Queue<DataFrame> inputQueue;
    for (auto& inDataFrame : inputData)
        inputQueue.waitAndPush(inDataFrame);

    Parallel::Crc8Wrapper crchasher;
    crchasher.calculateForWholeQueue({.src = inputQueue,
                                      .dest = outputQueue,
                                      .hasProducerFinished = hasProducerFinished,
                                      .tasksCount = threadCnt,
                                      .pool = pool});
    hasProducerFinished->store(true);
    crchasher.joinAndRethrowExceptions();

    std::set<DataFrame> result;
    DataFrame frame;
    while (outputQueue.tryPop(frame))
        result.insert(frame);

    std::set<DataFrame> expected;
    for (auto& outDataFrame : outputData)
        expected.insert(outDataFrame);

    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
}
} // namespace

BOOST_AUTO_TEST_SUITE(CrcHasherTestSuite)
BOOST_AUTO_TEST_CASE(CalculateByteArrayCrc)
{
    {
        std::vector<unsigned char> input{};
        BOOST_CHECK_EQUAL(0x0, crc8({input.data(), input.data() + input.size()}));
    }
    {
        std::vector<unsigned char> input{std::numeric_limits<unsigned char>::max()};
        BOOST_CHECK_EQUAL(0xAC, crc8({input.data(), input.data() + input.size()}));
    }
    {
        std::vector<unsigned char> input{std::numeric_limits<unsigned char>::min()};
        BOOST_CHECK_EQUAL(0x0, crc8({input.data(), input.data() + input.size()}));
    }
    {
        std::vector<unsigned char> input{0x2A};
        BOOST_CHECK_EQUAL(0x5D, crc8({input.data(), input.data() + input.size()}));
    }
    {
        std::vector<unsigned char> input{0xDA, 0x35, 0xFF, 0x23, 0x00, 0x04, 0x43};
        BOOST_CHECK_EQUAL(0x47, crc8({input.data(), input.data() + input.size()}));
    }
}

BOOST_AUTO_TEST_CASE(CalculatateFrameCrc)
{
    auto getPool = std::make_shared<Parallel::LazyMemoryPool>;

    // empty frame
    BOOST_CHECK_EQUAL(
        calculateCrc8OfFrame(DataFrame({.firstBlockIdx = 22, .blockSize = 1, .blocksCount = 0}),
                             getPool()),
        DataFrame({.firstBlockIdx = 22, .blockSize = 1, .blocksCount = 0}));

    // frame with one one-byte-size block
    BOOST_CHECK_EQUAL(calculateCrc8OfFrame(createDataFrameWithData(240, {{0x7B}}), getPool()),
                      createDataFrameWithData(240, {{0x12}}));

    // frame with several 1-byte-size blocks
    BOOST_CHECK_EQUAL(
        calculateCrc8OfFrame(createDataFrameWithData(773, {{0x7B}, {0x32}, {0x00}, {0x0C}}),
                             getPool()),
        createDataFrameWithData(773, {{0x12}, {0xA7}, {0x00}, {0x7D}}));

    // frame with one several-byte-size block
    BOOST_CHECK_EQUAL(
        calculateCrc8OfFrame(createDataFrameWithData(6, {{0x02, 0xFF, 0xAB}}), getPool()),
        createDataFrameWithData(6, {{0x1B}}));

    // frame with several several-byte-size blocks
    auto inputFrame = createDataFrameWithData(26789, {{0x02, 0xFF}, {0x3A, 0xAB}, {0xDE, 0x0C}});
    auto expectedFrame = createDataFrameWithData(26789, {{0x75}, {0x4A}, {0xD4}});
    BOOST_CHECK_EQUAL(calculateCrc8OfFrame(inputFrame, getPool()), expectedFrame);
}

BOOST_AUTO_TEST_CASE(CalculateForWholeQueue)
{
    testCalculateForWholeQueue({}, {}, 1);
    testCalculateForWholeQueue(getDefaultCrcInputData(), getDefaultExpectedCrcOutputData(), 1);
    testCalculateForWholeQueue(getDefaultCrcInputData(), getDefaultExpectedCrcOutputData(), 5);
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace Test
