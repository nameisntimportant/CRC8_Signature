#define BOOST_USE_ASAN

#include "memorysizeliterals.h"
#include "programmoptions.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace
{
const std::unordered_map<std::string, size_t> FromLiteralsToNumber{
    {"KB", KB}, {"MB", MB}, {"GB", GB}};
}

size_t parseMemorySize(std::string_view str)
{
    if (str.empty())
        throw po::error("can't parse empty string");

    auto literalsBeginIt =
        std::find_if_not(str.begin(), str.end(), [](const char ch) { return isdigit(ch); });

    const std::string intPart{str.begin(), literalsBeginIt};

    if (literalsBeginIt == str.end())
        return atoi(intPart.c_str());

    if (literalsBeginIt != std::prev(std::prev(str.end())))
        throw po::error("literals allowed only at the end of memory size string");

    if (literalsBeginIt == str.begin())
        throw po::error("can't parse string which contains only literals");

    const std::string literalPart{literalsBeginIt, str.end()};

    if (FromLiteralsToNumber.count(literalPart) == 0)
        throw po::error("unknown literals: " + literalPart);

    return atoi(intPart.c_str()) * FromLiteralsToNumber.at(literalPart);
}

std::variant<Options, std::string> getOptionsOrHelpStr(int argc, char const* argv[])
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "show help")
        ("input-file,i", po::value<std::string>()->required(), "input file path")
        ("output-file,o", po::value<std::string>()->required(), "output file path")
        ("size-of-block,s",po::value<std::string>()->default_value("1MB"),
         "size of hash calculating block in bytes. "
         "Supports KB, MB, GB literals.")
        ("type-of-disk,t",
         po::value<std::string>()->default_value("HDD"),
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

    const auto hardDiskType = vm.at("type-of-disk").as<std::string>();
    if (hardDiskType != "SSD" && hardDiskType != "HDD")
        throw po::error("wrong hard disk type: " + hardDiskType + ". Correct values: HDD, SSD");

    return Options{.inputFile = vm.at("input-file").as<std::string>(),
                   .outputFile = vm.at("output-file").as<std::string>(),
                   .blockSize = parseMemorySize(vm.at("size-of-block").as<std::string>()),
                   .isSSD = hardDiskType == "SSD",
                   .maxRamSize = parseMemorySize(vm.at("max-ram-size").as<std::string>())};
}
