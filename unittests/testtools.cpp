#include "testtools.h"
#include "dataframe.h"
#include "crchasher.h"

#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using iob = std::ios_base;

bool operator==(const DataFrame& lhs, const DataFrame& rhs)
{
    return lhs.blockSize() == rhs.blockSize() && lhs.totalSizeOfAllBlocks() == rhs.totalSizeOfAllBlocks() &&
           lhs.blocksCount() == rhs.blocksCount() && std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

bool operator!=(const DataFrame& lhs, const DataFrame& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& stream, const DataFrame& frame)
{
    stream << "block size: " << frame.blockSize() << " totalSizeOfAllBlocks: " << frame.totalSizeOfAllBlocks()
           << " blocksCount: " << frame.blocksCount() << " data: ";

    for (auto byte : boost::iterator_range<ConstDataIterator>{frame.cbegin(), frame.cend()})
        stream << std::hex << static_cast<int>(byte) << ' ';

    return stream;
}

bool operator<(const DataFrame& lhs, const DataFrame& rhs)
{
    return lhs.firstBlockIndex() < rhs.firstBlockIndex();
}

namespace Test
{
DataFrame createDataFrameWithData(size_t thirstBlockIndex,
                                  const std::vector<std::vector<unsigned char>> inputBlocks)
{
    auto blocksCount = inputBlocks.size();

    if (blocksCount == 0)
        return DataFrame({.firstBlockIdx = thirstBlockIndex, .blockSize = 1, .blocksCount = 0});

    auto blockSize = inputBlocks.front().size();

    DataFrame result(
        {.firstBlockIdx = thirstBlockIndex, .blockSize = blockSize, .blocksCount = blocksCount});

    for (size_t i = 0; i < blocksCount; i++)
    {
        assert(inputBlocks[i].size() == blockSize);
        copy(inputBlocks[i].begin(), inputBlocks[i].end(), result.blockAsRange(i).begin());
    }

    return result;
}

AutoFileRemover createAutoRemovableFileWithContent(
    const std::string& filePath, const std::vector<std::vector<unsigned char>>& content)
{
    assert(!fs::exists(filePath));

    std::ofstream writeStream(filePath, iob::binary | iob::out);
    for (auto& block : content)
        writeStream.write(reinterpret_cast<const char*>(block.data()), block.size());
    writeStream.close();

    assert(fs::exists(filePath));

    return AutoFileRemover(filePath);
}

std::vector<unsigned char> readWholeFile(const std::string& fileName)
{
    std::ifstream readStream(fileName, iob::binary | iob::in);
    std::vector<unsigned char> result(fs::file_size(fileName));
    readStream.read(reinterpret_cast<char*>(result.data()), result.size());
    return result;
}

std::vector<unsigned char> simpleCalculateCrcSignatureOfFile(const std::string& fileName,
                                                             const size_t blockSize)
{
    std::ifstream input(fileName, iob::binary | iob::in);
    std::vector<unsigned char> result;
    while (true)
    {
        std::vector<unsigned char> block(blockSize, 0);
        input.read(reinterpret_cast<char*>(block.data()), blockSize);

        if (input.gcount() == 0)
            break;

        result.push_back(crc8({block.data(), block.data() + block.size()}));
    }
    return result;
}
} // namespace Test
