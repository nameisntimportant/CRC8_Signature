#pragma once

#include "dataframe.h"

#include <filesystem>

namespace Test
{
class AutoFileRemover
{
public:
    explicit AutoFileRemover(const std::string_view filePath)
        : filePath_(filePath){};
    ~AutoFileRemover()
    {
        if (std::filesystem::exists(filePath_))
            std::filesystem::remove(filePath_);
    }

private:
    std::string filePath_;
};

AutoFileRemover createAutoRemovableFileWithContent(
    const std::string& filePath, const std::vector<std::vector<unsigned char>>& content);

std::vector<unsigned char> readWholeFile(const std::string& fileName);

std::vector<unsigned char> simpleCalculateCrcSignatureOfFile(const std::string& fileName,
                                                             const size_t blockSize);

DataFrame createDataFrameWithData(size_t thirstBlockIndex,
                                  const std::vector<std::vector<unsigned char>> inputBlocks);
} // namespace Test

// NOTE: those operators are used in several test suites, so we define it in testtools
bool operator==(const DataFrame& lhs, const DataFrame& rhs);
bool operator!=(const DataFrame& lhs, const DataFrame& rhs);
std::ostream& operator<<(std::ostream& stream, const DataFrame& frame);
bool operator<(const DataFrame& lhs, const DataFrame& rhs);
