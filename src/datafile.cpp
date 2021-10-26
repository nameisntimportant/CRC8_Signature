#include "datafile.h"
#include "memorysizeliterals.h"
#include "utils.h"

namespace
{
constexpr auto OptimalFstreamBufSize = MB;
} // namespace

DataFile::DataFile(const std::string& path, std::ios_base::openmode mode)
    : fileStreamBuf_(OptimalFstreamBufSize)
{
    fileStream_.rdbuf()->pubsetbuf(fileStreamBuf_.data(), fileStreamBuf_.size());
    fileStream_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fileStream_.open(path, mode);
}

DataFrame DataFile::readDataBlocksAsFrame(DataFrameConfig config)
{
    DataFrame frame{std::move(config)};
    fileStream_.seekg(frame.firstBlockIndex() * frame.blockSize());

    // TODO: add filereading errors handling
    try
    {
        fileStream_.read(frame.data(), frame.totalSizeOfAllBlocks());
    }
    catch (std::ifstream::failure& e)
    {
        // NOTE: ofstream sets failbit|eofbit when reachs the end of file. Since we set
        // exceptions(ifstream::failbit | ifstream::badbit), that leads to exception, but it's ok in
        // the programs workflow. So we supress this exception and rethrow others
        if (fileStream_.fail() && fileStream_.eof())
            fileStream_.clear();
        else
            throw;
    }

    const size_t readed = fileStream_.gcount();
    if (readed != frame.totalSizeOfAllBlocks())
        frame.setBlocksCount(ceilDevision(readed, frame.blockSize()));

    return frame;
}

void DataFile::writeDataFrame(const DataFrame& frame, const uintmax_t writingPosShift)
{
    fileStream_.seekp(writingPosShift + frame.firstBlockIndex() * frame.blockSize());
    fileStream_.write(frame.data(), frame.totalSizeOfAllBlocks());
}

DataFile::~DataFile()
{
    fileStream_.close();
}
