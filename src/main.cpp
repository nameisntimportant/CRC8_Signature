#include "crcsignatureoffile.h"
#include "iostream"
#include "programmoptions.h"

#include <boost/program_options.hpp>

namespace
{
void exitWithMessage(const std::string_view msg, int returnCode)
{
    std::cout << msg << std::endl;
    exit(returnCode);
}
} // namespace

int main(int argc, char const* argv[])
{
    try
    {
        std::variant<Options, std::string> optionsOrHelpStr;
        optionsOrHelpStr = getOptionsOrHelpStr(argc, argv);

        if (std::holds_alternative<std::string>(optionsOrHelpStr))
            exitWithMessage(std::get<std::string>(optionsOrHelpStr), 0);

        CrcSignatureOfFile crcSignatureOfFile(std::get<Options>(optionsOrHelpStr));
        crcSignatureOfFile.readCalculateAndWrite();
    }
    catch (boost::program_options::error& e)
    {
        exitWithMessage("Error during parsing command line options:\n\"" + std::string(e.what()) +
                            "\"\nPlease, use -h command line option to see help",
                        1);
    }
    catch (std::bad_alloc& e)
    {
        exitWithMessage("RAM related error:\n" + std::string(e.what()) +
                            "\n Please close other programms and try again",
                        2);
    }
    catch (std::exception& e)
    {
        exitWithMessage("Error during the program execution:\n" + std::string(e.what()), 3);
    }
    catch (...)
    {
        exitWithMessage("Unknown error during the program execution", 4);
    }

    return 0;
}
