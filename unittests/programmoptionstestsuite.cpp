#include "memorysizeliterals.h"
#include "programmoptions.h"

#include <boost/program_options.hpp>
#include <boost/test/unit_test.hpp>

#include <vector>

namespace po = boost::program_options;

bool operator==(const Options& lhs, const Options& rhs)
{
    return lhs.blockSize == rhs.blockSize && lhs.inputFile == rhs.inputFile &&
           lhs.isSSD == rhs.isSSD && lhs.outputFile == rhs.outputFile &&
           lhs.maxRamSize == rhs.maxRamSize;
}

std::ostream& operator<<(std::ostream& stream, const Options& options)
{
    return stream << "block size: " << options.blockSize << " input file: " << options.inputFile
                  << " isSSD: " << options.isSSD << " outputFile: " << options.outputFile
                  << " maxRamSize: " << options.maxRamSize;
}

namespace Test
{
BOOST_AUTO_TEST_SUITE(ProgramOptionsTestSuite)
BOOST_AUTO_TEST_CASE(ParseMemorySizeTest)
{
    BOOST_CHECK_EQUAL(0, parseMemorySize("0"));
    BOOST_CHECK_EQUAL(0, parseMemorySize("0KB"));
    BOOST_CHECK_EQUAL(0, parseMemorySize("000MB"));
    BOOST_CHECK_EQUAL(0, parseMemorySize("00GB"));

    BOOST_CHECK_EQUAL(9934871, parseMemorySize("9934871"));

    BOOST_CHECK_EQUAL(1 * KB, parseMemorySize("1KB"));
    BOOST_CHECK_EQUAL(675 * KB, parseMemorySize("675KB"));

    BOOST_CHECK_EQUAL(1 * MB, parseMemorySize("1MB"));
    BOOST_CHECK_EQUAL(55863 * MB, parseMemorySize("55863MB"));

    BOOST_CHECK_EQUAL(1 * GB, parseMemorySize("1GB"));
    BOOST_CHECK_EQUAL(25 * GB, parseMemorySize("25GB"));

    BOOST_CHECK_EXCEPTION(parseMemorySize(""), po::error, [](const po::error& e) {
        return std::string(e.what()) == "can't parse empty string";
    });

    BOOST_CHECK_EXCEPTION(parseMemorySize("-124"), po::error, [](const po::error& e) {
        return std::string(e.what()) == "literals allowed only at the end of memory size string";
    });

    BOOST_CHECK_EXCEPTION(parseMemorySize("1GG"), po::error, [](const po::error& e) {
        return std::string(e.what()) == "unknown literals: GG";
    });

    BOOST_CHECK_EXCEPTION(parseMemorySize("12MB10"), po::error, [](const po::error& e) {
        return std::string(e.what()) == "literals allowed only at the end of memory size string";
    });

    BOOST_CHECK_EXCEPTION(parseMemorySize("82MBGB"), po::error, [](const po::error& e) {
        return std::string(e.what()) == "literals allowed only at the end of memory size string";
    });

    BOOST_CHECK_EXCEPTION(parseMemorySize("MB"), po::error, [](const po::error& e) {
        return std::string(e.what()) == "can't parse string which contains only literals";
    });
}

// NOTE: we check only basic cases since most part of the work is done by boost::program_options
// and we assume that it's reliable
BOOST_AUTO_TEST_CASE(ShortNamesOfCommandLineParams)
{
    char const* input[6] = {"doesntmatter",
                            "-isome/folder/somefile.in",
                            "-oanother/folder/anotherfile.out",
                            "-tSSD",
                            "-s400MB",
                            "-m4GB"};

    Options expected{.inputFile = "some/folder/somefile.in",
                     .outputFile = "another/folder/anotherfile.out",
                     .blockSize = 400 * MB,
                     .isSSD = true,
                     .maxRamSize = 4 * GB};

    BOOST_CHECK_EQUAL(expected, std::get<Options>(getOptionsOrHelpStr(6, input)));
}

BOOST_AUTO_TEST_CASE(FullNamesOfCommandLineParams)
{
    char const* input[6] = {"doesntmatter",
                            "--output=anotherfile.out",
                            "--type=HDD",
                            "--size=2MB",
                            "--input=somefile.in",
                            "--max-ram-size=3MB"};

    Options expected{.inputFile = "somefile.in",
                     .outputFile = "anotherfile.out",
                     .blockSize = 2 * MB,
                     .isSSD = false,
                     .maxRamSize = 3 * MB};

    BOOST_CHECK_EQUAL(expected, std::get<Options>(getOptionsOrHelpStr(6, input)));
}

BOOST_AUTO_TEST_CASE(DefaultValues)
{
    char const* input[3] = {
        "doesntmatter", "-oanother/folder/anotherfile.out", "-isome/folder/somefile.in"};

    Options expected{.inputFile = "some/folder/somefile.in",
                     .outputFile = "another/folder/anotherfile.out",
                     .blockSize = 1 * MB,
                     .isSSD = false,
                     .maxRamSize = 3 * GB};

    BOOST_CHECK_EQUAL(expected, std::get<Options>(getOptionsOrHelpStr(3, input)));
}
BOOST_AUTO_TEST_SUITE_END()
} // namespace Test
