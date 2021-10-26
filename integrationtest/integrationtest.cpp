#include "crchasher.h"
#include "profiler.h"
#include "programmoptions.h"

#include <boost/process/child.hpp>
#include <boost/program_options.hpp>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
namespace po = boost::program_options;
namespace bp = boost::process;
using iob = std::ios_base;

namespace IntegrationTest
{

void exitWithMessage(const std::string_view msg, int returnCode)
{
    std::cout << msg << std::endl;
    exit(returnCode);
}

void ensureDoesntExists(const std::vector<std::string_view>& fileNames)
{
    for (auto fileName : fileNames)
    {
        if (fs::exists(fileName))
            exitWithMessage("pls remove file: " + std::string(fileName), 1);
    }
}

void remove(const std::vector<std::string_view>& fileNames)
{
    for (auto fileName : fileNames)
        fs::remove(std::string(fileName).c_str());
}

struct Options
{
    std::string pathToTestedProgram;
    std::string inputFile;
    size_t blockSize;
    std::string hardDiskType;
    size_t maxRamSize;
};

std::variant<Options, std::string> getOptionsOrHelpStr(int argc, char const* argv[])
{
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "show help")
    ("path-to-program,p", po::value<std::string>()->required(), "path to the tested program (CRC_8_signature).")
    ("input-file-path,i", po::value<std::string>()->required(), "path to the input file")
    ("size-of-block,s",po::value<std::string>()->default_value("1MB"),"size of hash calculating block in bytes. "
     "Supports KB, MB, GB literals.")
    ("type-of-disk,t", po::value<std::string>()->default_value("HDD"),
     "type of hard disk, needed in order to optimise perfomance. "
     "Possible values: HDD, SSD")
    ("max-ram-size,m",
     po::value<std::string>()->default_value("3GB"),
     "maximum size of RAM that will be used by the programm."
     "Supports KB, MB, GB literals.");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::ostringstream helpMessage;
        helpMessage << desc;
        return helpMessage.str();
    }

    return Options{.pathToTestedProgram = vm.at("path-to-program").as<std::string>(),
                   .inputFile = vm.at("input-file-path").as<std::string>(),
                   .blockSize = parseMemorySize(vm.at("size-of-block").as<std::string>()),
                   .hardDiskType = (vm.at("type-of-disk").as<std::string>()),
                   .maxRamSize = parseMemorySize(vm.at("max-ram-size").as<std::string>())};
}

void calculateCrc8WithProgramUnderTheTest(const std::string& inputFileName,
                                          const Options& options,
                                          const std::string& testedProgramOutputFileName)
{
    LOG_DURATION("program under the test calculation");
    std::cout << "calculation with program under the test (CRC_8_signature) is started" << std::endl;

    const std::string params = "-i" + inputFileName + " -o" + testedProgramOutputFileName + " -s" +
                          std::to_string(options.blockSize) + " -t" + options.hardDiskType + " -m" +
                          std::to_string(options.maxRamSize);
    bp::child calculation(options.pathToTestedProgram + ' ' + params);
    calculation.wait();

    if (calculation.exit_code())
    {
        exitWithMessage("crc8 calculation with tested program failed. Returning code: " +
                            std::to_string(calculation.exit_code()),
                        3);
    }

    std::cout << "calculation with program under the test (CRC_8_signature) is finished" << std::endl;
}

void calculateCrc8WithSimpleAlghoritm(const std::string& inputFileName,
                                      const Options& options,
                                      const std::string& simpleAlghoritmOutputFileName)
{
    LOG_DURATION("simple calculation");
    std::cout << "calculation with simple and slow alghoritm is started" << std::endl;

    std::ifstream input(inputFileName, iob::binary | iob::in);
    std::ofstream output(simpleAlghoritmOutputFileName, iob::binary | iob::out);
    while (true)
    {
        std::vector<unsigned char> block(options.blockSize, 0);
        input.read(reinterpret_cast<char*>(block.data()), options.blockSize);

        if (input.gcount() == 0)
            break;
        auto crc = crc8({block.data(), block.data() + block.size()});
        output.write(reinterpret_cast<char*>(&crc), sizeof(unsigned char));
    }

    std::cout << "calculation with simple and slow alghoritm is finished" << std::endl;
}

int compareFiles(const std::string& lhs, const std::string& rhs)
{
    bp::child diffProcess("/bin/diff " + lhs + ' ' + rhs);
    diffProcess.wait();
    return diffProcess.exit_code();
}
} // namespace IntegrationTest

int main(int argc, char const* argv[])
{
    using namespace IntegrationTest;

    std::variant<IntegrationTest::Options, std::string> optionsOrHelpStr;
    optionsOrHelpStr = IntegrationTest::getOptionsOrHelpStr(argc, argv);

    if (std::holds_alternative<std::string>(optionsOrHelpStr))
        exitWithMessage(std::get<std::string>(optionsOrHelpStr), 0);
    const auto options = std::get<IntegrationTest::Options>(optionsOrHelpStr);

    const auto testedProgramOutputFileName = "tempActualOutput";
    const auto simpleAlghoritmOutputFileName = "tempExpectedOutput";

    ensureDoesntExists({testedProgramOutputFileName, simpleAlghoritmOutputFileName});

    calculateCrc8WithProgramUnderTheTest(options.inputFile, options, testedProgramOutputFileName);
    calculateCrc8WithSimpleAlghoritm(options.inputFile, options, simpleAlghoritmOutputFileName);

    if (compareFiles(testedProgramOutputFileName, simpleAlghoritmOutputFileName))
        exitWithMessage("Output files are not equal! They will be stored to further debugging", 4);
    else
        std::cout << "Output files are equal. Test is PASSED" << std::endl;

    remove({testedProgramOutputFileName, simpleAlghoritmOutputFileName});

    return 0;
}
