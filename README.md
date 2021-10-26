# Overview
The command-line program which generates the CRC8 signature of a given file using several threads.
The signature is generated as follows: the source file is divided into equal (fixed) length blocks. When the source file size is not divisible by block size, the last fragment complemented with zeroes to full block size. A hash value is calculated for each block and then added to the output signature file. 

# Command line parametrs
 - -i input file path
 - -o output file path
 - -s block size (1MB by default)
 - -t disk type (HDD or SSD. HDD by default)
