#pragma once

#include <string>
#include <variant>

struct Options
{
    std::string inputFile;
    std::string outputFile;
    size_t blockSize;
    bool isSSD;
    size_t maxRamSize;
};

std::variant<Options, std::string> getOptionsOrHelpStr(int argc, char const* argv[]);

size_t parseMemorySize(std::string_view str);
