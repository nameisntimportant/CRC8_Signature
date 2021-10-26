#include <boost/test/unit_test.hpp>

#include <filesystem>

#include "datafile.h"
#include "testdefs.h"
#include "testtools.h"

namespace fs = std::filesystem;
using iob = std::ios_base;

namespace Test
{
BOOST_AUTO_TEST_SUITE(DataFileTestSuite)
BOOST_AUTO_TEST_CASE(ReadDataBlocksAsFrameFromEmptyFileTest)
{
    auto fileRemover = createAutoRemovableFileWithContent(TempTestFileName, {});

    DataFile dataFile(TempTestFileName, iob::binary | iob::in);
    assert(fs::exists(TempTestFileName));

    auto zeroBlockReadingResult =
        dataFile.readDataBlocksAsFrame({.firstBlockIdx = 0, .blockSize = 1, .blocksCount = 0});
    BOOST_CHECK_EQUAL(createDataFrameWithData(0, {}), zeroBlockReadingResult);

    auto severalBlocksReadingResult =
        dataFile.readDataBlocksAsFrame({.firstBlockIdx = 0, .blockSize = 1, .blocksCount = 4});
    BOOST_CHECK_EQUAL(createDataFrameWithData(0, {}), severalBlocksReadingResult);
}

BOOST_AUTO_TEST_CASE(ReadDataBlocksAsFrameFromFileWithOneDataBlockTest)
{
    const std::vector<std::vector<unsigned char>> data = {{0x77, 0x22, 0xAB, 0xCC, 0x01}};
    auto fileRemover = createAutoRemovableFileWithContent(TempTestFileName, data);
    assert(fs::exists(TempTestFileName));

    DataFile dataFile(TempTestFileName, iob::binary | iob::in);

    auto readStartingFromZeroBlockResult =
        dataFile.readDataBlocksAsFrame({.firstBlockIdx = 0, .blockSize = 5, .blocksCount = 1});
    BOOST_CHECK_EQUAL(createDataFrameWithData(0, data), readStartingFromZeroBlockResult);

    auto readStartingFromNonZeroBlockResult =
        dataFile.readDataBlocksAsFrame({.firstBlockIdx = 3, .blockSize = 5, .blocksCount = 1});
    BOOST_CHECK_EQUAL(
        DataFrame({DataFrameConfig{.firstBlockIdx = 3, .blockSize = 5, .blocksCount = 0}}),
        readStartingFromNonZeroBlockResult);
}

BOOST_AUTO_TEST_CASE(ReadDataBlocksAsFrameFromFileWithSeveralDataBlocksTest)
{
    const std::vector<std::vector<unsigned char>> data = {
        {0x77, 0x22}, {0xAB, 0xCC}, {0x01, 0xF2}, {0x9A, 0x21}};
    auto fileRemover = createAutoRemovableFileWithContent(TempTestFileName, data);

    DataFile dataFile(TempTestFileName, iob::binary | iob::in);

    auto readStartingFromZeroBlockResult =
        dataFile.readDataBlocksAsFrame({.firstBlockIdx = 0, .blockSize = 2, .blocksCount = 4});
    BOOST_CHECK_EQUAL(createDataFrameWithData(0, data), readStartingFromZeroBlockResult);

    auto readStartingFromNonZeroBlockResult =
        dataFile.readDataBlocksAsFrame({.firstBlockIdx = 2, .blockSize = 2, .blocksCount = 2});
    BOOST_CHECK_EQUAL(createDataFrameWithData(2, {data.at(2), data.at(3)}),
                      readStartingFromNonZeroBlockResult);

    auto readMoreBlocksThenFileContainsResult =
        dataFile.readDataBlocksAsFrame({.firstBlockIdx = 1, .blockSize = 2, .blocksCount = 7});
    BOOST_CHECK_EQUAL(createDataFrameWithData(1, {data.at(1), data.at(2), data.at(3)}),
                      readMoreBlocksThenFileContainsResult);

    auto readLessBlocksThenFileContainsResult =
        dataFile.readDataBlocksAsFrame({.firstBlockIdx = 1, .blockSize = 2, .blocksCount = 2});
    BOOST_CHECK_EQUAL(createDataFrameWithData(1, {data.at(1), data.at(2)}),
                      readLessBlocksThenFileContainsResult);
}

BOOST_AUTO_TEST_CASE(ReadDataBlocksAsFrameFromFileWithNonIntegerNumberOfDataBlockTest)
{
    const std::vector<std::vector<unsigned char>> data = {{0x77, 0x22, 0xAB}, {0x01}};
    auto fileRemover = createAutoRemovableFileWithContent(TempTestFileName, data);

    DataFile dataFile(TempTestFileName, iob::binary | iob::in);

    // NOTE: Expect that the end of last datablock will be zero-filled
    auto result =
        dataFile.readDataBlocksAsFrame({.firstBlockIdx = 0, .blockSize = 3, .blocksCount = 4});
    auto expectedDataFrameWithZeroFilledLastDataBlock =
        createDataFrameWithData(0, {{0x77, 0x22, 0xAB}, {0x01, 0x00, 0x00}});
    BOOST_CHECK_EQUAL(expectedDataFrameWithZeroFilledLastDataBlock, result);
}

BOOST_AUTO_TEST_CASE(WriteEmptyDataFrameToNonExistingFileTest)
{
    assert(!fs::exists(TempTestFileName));
    AutoFileRemover remover(TempTestFileName);

    {
        // NOTE: we need braces to force DataFile to close
        DataFile dataFile(TempTestFileName, iob::binary | iob::out);
        dataFile.writeDataFrame(createDataFrameWithData(10, {}), 11);
    }

    BOOST_VERIFY(fs::exists(TempTestFileName));
    BOOST_CHECK_EQUAL(fs::file_size(TempTestFileName), 0);
}

BOOST_AUTO_TEST_CASE(WriteNonEmptyDataFrameToNonExistingFileTest)
{
    std::vector<std::vector<unsigned char>> data = {{0x71, 0xAA}, {0xF2, 0x23}};
    AutoFileRemover remover(TempTestFileName);

    {
        DataFile dataFile(TempTestFileName, iob::binary | iob::out);
        dataFile.writeDataFrame(createDataFrameWithData(0, data));
    }

    const auto fileContent = readWholeFile(TempTestFileName);
    const auto expectedContent = {0x71, 0xAA, 0xF2, 0x23};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        fileContent.begin(), fileContent.end(), expectedContent.begin(), expectedContent.end());
}

BOOST_AUTO_TEST_CASE(WriteNonEmptyDataFrameFrom3thBlockToNonExistingFileTest)
{
    std::vector<std::vector<unsigned char>> data = {{0x71, 0xAA}, {0xF2, 0x23}};
    AutoFileRemover remover(TempTestFileName);

    {
        DataFile dataFile(TempTestFileName, iob::binary | iob::out);
        dataFile.writeDataFrame(createDataFrameWithData(3, data));
    }

    const auto fileContent = readWholeFile(TempTestFileName);
    std::vector<unsigned char> expectedFileContent = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x071, 0xAA, 0xF2, 0x23};
    BOOST_CHECK_EQUAL_COLLECTIONS(fileContent.begin(),
                                  fileContent.end(),
                                  expectedFileContent.begin(),
                                  expectedFileContent.end());
}

BOOST_AUTO_TEST_CASE(WriteNonEmptyDataFrameFrom3thBlockWith2BytesShiftToNonExistingFileTest)
{
    std::vector<std::vector<unsigned char>> data = {{0x71, 0xAA}, {0xF2, 0x23}};
    AutoFileRemover remover(TempTestFileName);

    {
        DataFile dataFile(TempTestFileName, iob::binary | iob::out);
        dataFile.writeDataFrame(createDataFrameWithData(3, data), 2);
    }

    const auto fileContent = readWholeFile(TempTestFileName);
    std::vector<unsigned char> expectedFileContent = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x071, 0xAA, 0xF2, 0x23};
    BOOST_CHECK_EQUAL_COLLECTIONS(fileContent.begin(),
                                  fileContent.end(),
                                  expectedFileContent.begin(),
                                  expectedFileContent.end());
}

BOOST_AUTO_TEST_CASE(WriteEmptyDataFrameToExistingFileTest)
{
    auto fileRemover = createAutoRemovableFileWithContent(
        TempTestFileName, {{0x12, 0x30}, {0x45, 0x60}, {0x0A, 0x1C}, {0x71, 0xAA}, {0xF2, 0x23}});

    {
        DataFile dataFile(TempTestFileName, iob::binary | iob::out | iob::in);
        dataFile.writeDataFrame(createDataFrameWithData(10, {}), 11);
    }

    const auto fileContent = readWholeFile(TempTestFileName);
    const auto expectedContent = {0x12, 0x30, 0x45, 0x60, 0x0A, 0x1C, 0x71, 0xAA, 0xF2, 0x23};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        fileContent.begin(), fileContent.end(), expectedContent.begin(), expectedContent.end());
}

BOOST_AUTO_TEST_CASE(WriteNonEmptyDataFrameToExistingFileTest)
{
    auto fileRemover = createAutoRemovableFileWithContent(
        TempTestFileName, {{0x12, 0x30}, {0x45, 0x60}, {0x0A, 0x1C}, {0x71, 0xAA}, {0xF2, 0x23}});

    {
        DataFile dataFile(TempTestFileName, iob::binary | iob::out | iob::in);
        dataFile.writeDataFrame(createDataFrameWithData(0, {{0xAA, 0xAB}, {0xCA, 0xCB}}));
    }

    const auto fileContent = readWholeFile(TempTestFileName);
    const auto expectedContent = {0xAA, 0xAB, 0xCA, 0xCB, 0x0A, 0x1C, 0x71, 0xAA, 0xF2, 0x23};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        fileContent.begin(), fileContent.end(), expectedContent.begin(), expectedContent.end());
}

BOOST_AUTO_TEST_CASE(WriteNonEmptyDataFrameFrom4thBlockToExistingFileTest)
{
    auto fileRemover = createAutoRemovableFileWithContent(
        TempTestFileName, {{0x12, 0x30}, {0x45, 0x60}, {0x0A, 0x1C}, {0x71, 0xAA}, {0xF2, 0x23}});

    {
        DataFile dataFile(TempTestFileName, iob::binary | iob::out | iob::in);
        dataFile.writeDataFrame(createDataFrameWithData(4, {{0xAA, 0xAB}, {0xCA, 0xCB}}));
    }

    const auto fileContent = readWholeFile(TempTestFileName);
    std::vector<unsigned char> expectedFileContent = {
        0x12, 0x30, 0x45, 0x60, 0x0A, 0x1C, 0x71, 0xAA, 0xAA, 0xAB, 0xCA, 0xCB};
    BOOST_CHECK_EQUAL_COLLECTIONS(fileContent.begin(),
                                  fileContent.end(),
                                  expectedFileContent.begin(),
                                  expectedFileContent.end());
}

BOOST_AUTO_TEST_CASE(WriteNonEmptyDataFrameFromZeroBlockWith12BytesShiftToExistingFileTest)
{
    auto fileRemover = createAutoRemovableFileWithContent(
        TempTestFileName, {{0x12, 0x30}, {0x45, 0x60}, {0x0A, 0x1C}, {0x71, 0xAA}, {0xF2, 0x23}});

    {
        DataFile dataFile(TempTestFileName, iob::binary | iob::out | iob::in);
        dataFile.writeDataFrame(createDataFrameWithData(0, {{0xAA, 0xAB}}), 12);
    }

    const auto fileContent = readWholeFile(TempTestFileName);
    std::vector<unsigned char> expectedFileContent = {
        0x12, 0x30, 0x45, 0x60, 0x0A, 0x1C, 0x71, 0xAA, 0xF2, 0x23, 0x00, 0x00, 0xAA, 0xAB};
    BOOST_CHECK_EQUAL_COLLECTIONS(fileContent.begin(),
                                  fileContent.end(),
                                  expectedFileContent.begin(),
                                  expectedFileContent.end());
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace Test
