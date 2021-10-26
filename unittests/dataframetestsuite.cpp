#include <boost/test/unit_test.hpp>

#include "dataframe.h"
#include "testtools.h"

namespace Test
{
BOOST_AUTO_TEST_SUITE(DataFrameTestSuite)
BOOST_AUTO_TEST_CASE(DataFrameDefaultCtorTest)
{
    DataFrame frame;
    BOOST_CHECK_EQUAL(0, frame.firstBlockIndex());
    BOOST_CHECK_EQUAL(1, frame.blockSize());
    BOOST_CHECK_EQUAL(0, frame.blocksCount());
    BOOST_CHECK_EQUAL(0, frame.totalSizeOfAllBlocks());
}

BOOST_AUTO_TEST_CASE(DataFrameCtorWithParamsTest)
{
    DataFrame frame({.firstBlockIdx = 245, .blockSize = 91, .blocksCount = 242});
    BOOST_CHECK_EQUAL(245, frame.firstBlockIndex());
    BOOST_CHECK_EQUAL(91, frame.blockSize());
    BOOST_CHECK_EQUAL(242, frame.blocksCount());
    BOOST_CHECK_EQUAL(91u * 242u, frame.totalSizeOfAllBlocks());
    for (size_t i = 0; i < frame.blocksCount(); i++)
    {
        for (auto byte : frame.blockAsRange(i))
            BOOST_CHECK_EQUAL(byte, 0);
    }
}

struct DataFrameFixture
{
    const size_t blockSize = 3;
    std::vector<unsigned char> block0 = {0x03, 0xF1, 0x70};
    std::vector<unsigned char> block1 = {0x26, 0x11, 0xCC};
    std::vector<unsigned char> emptyBlock = {0x00, 0x00, 0x00};
    DataFrame frame = DataFrame({.firstBlockIdx = 11, .blockSize = blockSize, .blocksCount = 2});

    DataFrameFixture()
    {
        copy(block0.begin(), block0.end(), frame.blockAsRange(0).begin());
        copy(block1.begin(), block1.end(), frame.blockAsRange(1).begin());
    };
};

BOOST_FIXTURE_TEST_CASE(DataFrameAssignCtorTest, DataFrameFixture)
{
    BOOST_CHECK_EQUAL(DataFrame(frame), frame);
}

BOOST_FIXTURE_TEST_CASE(DataFrameMoveCtorTest, DataFrameFixture)
{
    DataFrame copiedFrame(frame);
    DataFrame ctoredFrame(std::move(frame));
    BOOST_CHECK_EQUAL(copiedFrame, ctoredFrame);
}

BOOST_FIXTURE_TEST_CASE(DataFrameCopyAssignTest, DataFrameFixture)
{
    DataFrame result;
    result = frame;
    BOOST_CHECK_EQUAL(result, frame);
}

BOOST_FIXTURE_TEST_CASE(DataFrameMoveAssignTest, DataFrameFixture)
{
    DataFrame copiedFrame(frame);
    DataFrame result = std::move(frame);
    BOOST_CHECK_EQUAL(copiedFrame, result);
}

BOOST_FIXTURE_TEST_CASE(DataFrameBlockAsRangeTest, DataFrameFixture)
{
    BOOST_CHECK_EQUAL_COLLECTIONS(
        frame.blockAsRange(0).begin(), frame.blockAsRange(0).end(), block0.begin(), block0.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(
        frame.blockAsRange(1).begin(), frame.blockAsRange(1).end(), block1.begin(), block1.end());
}

BOOST_FIXTURE_TEST_CASE(DataFrameSetZeroBlocksCountTest, DataFrameFixture)
{
    frame.setBlocksCount(0);
    BOOST_CHECK_EQUAL(0, frame.blocksCount());
    BOOST_CHECK_EQUAL(0, frame.totalSizeOfAllBlocks());
}

BOOST_FIXTURE_TEST_CASE(DataFrameSetDecreasedBlocksCountTest, DataFrameFixture)
{
    const auto decreasedBlocksNumber = 1;
    frame.setBlocksCount(decreasedBlocksNumber);

    BOOST_CHECK_EQUAL(decreasedBlocksNumber, frame.blocksCount());
    BOOST_CHECK_EQUAL(decreasedBlocksNumber * blockSize, frame.totalSizeOfAllBlocks());
    BOOST_CHECK_EQUAL_COLLECTIONS(
        frame.blockAsRange(0).begin(), frame.blockAsRange(0).end(), block0.begin(), block0.end());
}

BOOST_FIXTURE_TEST_CASE(DataFrameSetBlocksCountBiggerThenCapacityTest, DataFrameFixture)
{
    const auto tooBigBlocksNumber = 4;
    BOOST_CHECK_EXCEPTION(frame.setBlocksCount(tooBigBlocksNumber);, std::out_of_range, [](const std::out_of_range& e) {
        return std::string(e.what()) == "The size of the new number of blocks is larger than the capacity";
    });
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace Test
