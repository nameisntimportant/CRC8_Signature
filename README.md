# Overview
The command-line program which generates the CRC8 signature of a given file using several threads.
The signature is generated as follows: the source file is divided into equal (fixed) length blocks. When the source file size is not divisible by block size, the last fragment complemented with zeroes to full block size. A hash value is calculated for each block and then added to the output signature file. 

# Command line parametrs
 - -i input file path
 - -o output file path
 - -s block size (1MB by default)
 - -t disk type (HDD or SSD. HDD by default)
 - -m maximum RAM usage of the program (3GB by default)

# Implementation description
The code is written in such a way that it would be readable without documentation. However, since its main purpose is to demonstrate my capabilities to potential employers, a brief description of the code is provided below to help the reviewer process the code faster.
The program implements a reader-calculator-writer architecture with data transfer using a thread-safe queue. Working with threads is done using a thread pool.
##### Brief description of classes:
 - **DataFrame** - a fixed-size container for storing data blocks. Re-use memory using a thread-safe memory pool.
 - **Parallel::LazyMemoryPool** - a thread-safe wrapper over boost::pool<>() with lazy initialization.
 - **ZeroFilledMemory** - RAII wrapper over raw memory requested from the system or from the memory pool.
 - **DataFile** - implements working with a file as with a sequence of data blocks.
 - **Parallell::DataFileWrapper** - implements the functionality of asynchronous work with DataFile.
 - **Parallell::Crc8wrapper** - implements asynchronous CRC8 signature calculation.
 - **Parallel::Queue** - thread-safe wrapper over std::queue<> with a limit on the maximum number of elements.
 - **CrcSignatureOfFile** - owner of a thread pool, instances of reader (**Parallell::DataFileWrapper**), calculator (**Parallell::Crc8wrapper**) and writer (**Parallell::DataFileWrapper**) and the threadsafe queues.
