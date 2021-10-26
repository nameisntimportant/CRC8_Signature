#pragma once

#include "dataframe.h"

#include <fstream>

class DataFile
{
public:
    DataFile(const std::string& path, std::ios_base::openmode mode);
    DataFrame readDataBlocksAsFrame(DataFrameConfig config);
    void writeDataFrame(const DataFrame& data, uintmax_t writingPosShift = 0);
    ~DataFile();

private:
    std::fstream fileStream_;
    std::vector<char> fileStreamBuf_;
};
