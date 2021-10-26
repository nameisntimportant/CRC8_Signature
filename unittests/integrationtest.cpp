#include <boost/process/child.hpp>
#include <boost/process/search_path.hpp>
#include <boost/program_options.hpp>

#include <filesystem>

#include "programmoptions.h"
#include "simplecrcoffilecalculater.h"

using namespace std;
namespace fs = std::filesystem;
namespace po = boost::program_options;
namespace bp = boost::process;

namespace IntegrationTest
{

void exitWithMessage(const string_view msg, int returnCode)
{
    cout << msg << endl;
    exit(returnCode);
}

struct Options
{
    string pathToTestedProgram;
    size_t inputFileSize;
    size_t blockSize;
    string hardDiskType;
};

variant<Options, string> getOptionsOrHelpStr(int argc, char const* argv[])
{
    po::options_description desc(
        "The program's workflow:\n1. Generate a file with a set size filled with random data.\n2. "
        "Calculate crc8 signature using CRC_8_signature and store it in a file.\n3. Calculate crc8 "
        "signature of the file using slow and simple algorithm, store it in a file.\n4. Compare "
        "those two files using diff, print result of comparing. Return 0 and delete output files "
        "if files are equal.");
    desc.add_options()("help,h", "show help")("path-to-program,p",
                                              po::value<string>()->required(),
                                              "path to the tested program (CRC_8_signature).")(
        "input-file-size,i",
        po::value<string>()->required(),
        "the size of the automatically generated input file")(
        "size-of-block,s",
        po::value<string>()->default_value("1MB"),
        "size of hash calculating block in bytes. "
        "Supports KB, MB, GB literals.")(
        "type-of-disk,t",
        po::value<string>()->default_value("HDD"),
        "type of hard disk, needed in order to optimise perfomance. "
        "Possible values: HDD, SSD");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        ostringstream helpMessage;
        helpMessage << desc;
        return helpMessage.str();
    }

    return Options{.pathToTestedProgram = vm.at("path-to-program").as<string>(),
                   .inputFileSize = vm.at("input-file-size").as<size_t>(),
                   .blockSize = parseMemorySize(vm.at("size-of-block").as<string>()),
                   .hardDiskType = (vm.at("type-of-disk").as<string>())};
}

void generateInputFile(const string& dataBlockSizeStr, const string& inputFileName)
{
    cout << "generating of input file is started" << endl;

    const auto sizeParam = "$(("s + dataBlockSizeStr + ")"s;
    bp::child generatingProcess(bp::search_path("openssl"),
                                "rand -out "s + inputFileName + " -base64 " + sizeParam);
    generatingProcess.wait();
    if (!fs::exists(inputFileName))
        exitWithMessage("generating of input file failed: it doesn't exist", 2);

    cout << "generating of input file is finished" << endl;
}

void calculateCrc8WithProgramUnderTheTest(const std::string& inputFileName,
                                          const Options& options,
                                          const std::string& dataBlockSizeStr,
                                          const std::string& testedProgramOutputFileName)
{
    cout << "calculation with program under the test (CRC_8_signature) is started" << endl;

    bp::child calculation(bp::search_path(options.pathToTestedProgram),
                          "-i"s + inputFileName + "-o"s + testedProgramOutputFileName + "-s"s +
                              dataBlockSizeStr + "-t"s + options.hardDiskType);
    calculation.wait();
    if (calculation.exit_code())
    {
        exitWithMessage("crc8 calculation with tested program failed. Returning code: "s +
                            to_string(calculation.exit_code()),
                        3);
    }

    cout << "calculation with program under the test (CRC_8_signature) is finished" << endl;
}

void calculateCrc8WithSimpleAlghoritm(const std::string& inputFileName,
                                      const Options& options,
                                      const std::string& simpleAlghoritmOutputFileName)
{
    cout << "calculation with simple and slow alghoritm is started" << endl;

    std::ifstream input(inputFileName, std::ios_base::binary | std::ios_base::in);
    std::ofstream output(simpleAlghoritmOutputFileName, std::ios_base::binary | std::ios_base::out);
    while (true)
    {
        std::vector<unsigned char> block(options.blockSize, 0);
        input.read(reinterpret_cast<char*>(block.data()), options.blockSize);

        if (input.gcount() == 0)
            break;

        auto crc = crc8(block.data(), block.size());
        output.write(reinterpret_cast<char*>(crc), sizeof(unsigned char));
    }

    cout << "calculation with simple and slow alghoritm is finished" << endl;
}

int compareFiles(const std::string& lhs, const std::string& rhs)
{
    bp::child diffProcess(bp::search_path("diff"), lhs + " "s + rhs);
    diffProcess.wait();
    return diffProcess.exit_code();
}
} // namespace IntegrationTest


int main(int argc, char const* argv[])
{
    using namespace IntegrationTest;

    variant<IntegrationTest::Options, string> optionsOrHelpStr;
    optionsOrHelpStr = IntegrationTest::getOptionsOrHelpStr(argc, argv);

    if (holds_alternative<string>(optionsOrHelpStr))
        exitWithMessage(get<string>(optionsOrHelpStr), 0);
    const auto options = get<IntegrationTest::Options>(optionsOrHelpStr);

    const auto inputFileName = "tempFilePlsRemoveInput";
    const auto testedProgramOutputFileName = "tempFilePlsRemoveOutput";
    const auto simpleAlghoritmOutputFileName = "tempFilePlsRemoveOutputSimple";
    const auto dataBlockSizeStr = to_string(options.inputFileSize);

    if (fs::exists(inputFileName) || fs::exists(testedProgramOutputFileName))
        exitWithMessage("pls remove existing files: "s + inputFileName + ' ' +
                            testedProgramOutputFileName + ' ' + simpleAlghoritmOutputFileName,
                        1);

    generateInputFile(dataBlockSizeStr, inputFileName);

    calculateCrc8WithProgramUnderTheTest(
        inputFileName, options, dataBlockSizeStr, testedProgramOutputFileName);
    calculateCrc8WithSimpleAlghoritm(inputFileName, options, simpleAlghoritmOutputFileName);

    if (compareFiles(testedProgramOutputFileName, simpleAlghoritmOutputFileName))
        exitWithMessage("Files are not equal! They will be stored to further debugging"s, 4);

    remove(testedProgramOutputFileName);
    remove(simpleAlghoritmOutputFileName);
    remove(inputFileName);

    return 0;
}
