#include <boost/asio/post.hpp>
#include <boost/test/unit_test.hpp>

#include <filesystem>

#include "crcsignatureoffile.h"
#include "memorysizeliterals.h"
#include "testdefs.h"
#include "testtools.h"

namespace fs = std::filesystem;

namespace Test
{

BOOST_AUTO_TEST_SUITE(CrcSignatureOfFileTestSuite)
BOOST_AUTO_TEST_CASE(CleanupTest, *boost::unit_test::timeout(2))
{
    auto infinityTask = []() {
        while (true)
        {
        };
    };

    {
        auto fileRemover =
            createAutoRemovableFileWithContent(TempTestFileName, {{0x01, 0x02, 0x30}});
        boost::asio::thread_pool pool(1);
        post(pool, infinityTask);

        CrcSignatureOfFile::cleanup(pool, TempTestFileName, 2);

        auto fileContent = readWholeFile(TempTestFileName);
        auto expectedContent = {0x01, 0x02};
        BOOST_CHECK_EQUAL_COLLECTIONS(
            fileContent.begin(), fileContent.end(), expectedContent.begin(), expectedContent.end());
        pool.join();
    }
    {
        auto fileRemover =
            createAutoRemovableFileWithContent(TempTestFileName, {{0x01, 0x02, 0x30}});
        boost::asio::thread_pool pool(1);
        post(pool, infinityTask);

        CrcSignatureOfFile::cleanup(pool, TempTestFileName, std::nullopt);
        BOOST_VERIFY(!fs::exists(TempTestFileName));
        pool.join();
    }
    {
        boost::asio::thread_pool pool(1);
        post(pool, infinityTask);

        CrcSignatureOfFile::cleanup(pool, TempTestFileName, std::nullopt);
        BOOST_VERIFY(!fs::exists(TempTestFileName));
        pool.join();
    }
}

void testReadCalculateAndWrite(size_t dataBlockSize, bool isSSD, size_t maxRamSize)
{
    assert(!fs::exists(TempTestFileName));
    assert(fs::exists(PermanentTestFileName));
    AutoFileRemover remover(TempTestFileName);

    CrcSignatureOfFile calculater({.inputFile = PermanentTestFileName,
                                   .outputFile = TempTestFileName,
                                   .blockSize = dataBlockSize,
                                   .isSSD = isSSD,
                                   .maxRamSize = maxRamSize});
    calculater.readCalculateAndWrite();

    const auto result = readWholeFile(TempTestFileName);
    const auto expected = simpleCalculateCrcSignatureOfFile(PermanentTestFileName, dataBlockSize);

    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(ReadCalculateAndWriteTest)
{
    testReadCalculateAndWrite(1, true, 1 * MB);
    testReadCalculateAndWrite(20, false, 1 * MB);
    testReadCalculateAndWrite(12 * KB, false, 36 * KB);
    testReadCalculateAndWrite(MB, true, 300 * MB);
    testReadCalculateAndWrite(2.3 * MB, true, 300 * MB);

    BOOST_CHECK_EXCEPTION(testReadCalculateAndWrite(3 * MB, true, 1 * MB), std::invalid_argument, [](const std::invalid_argument& e) {
        return std::string(e.what()) == "Max RAM size is too small to proceed data blocks with such a size. "
        "Please, either reduce data block size, either increase max RAM size. Please run the "
        "program with --help parametr for more information";
    });
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace Test
