# Kanzi

Kanzi is a modern, modular, portable, and efficient lossless data compressor written in C++.

* Modern: Kanzi implements state-of-the-art compression algorithms and is built to fully utilize multi-core CPUs via built-in multi-threading.
* Modular: Entropy codecs and data transforms can be selected and combined at runtime to best suit the specific data being compressed.
* Portable: Supports a wide range of operating systems, compilers, and C++ standards (details below).
* Expandable: A clean, interface-driven design—with no external dependencies—makes Kanzi easy to integrate, extend, and customize.
* Efficient: Carefully optimized to balance compression ratio and speed for practical, high-performance usage.

Unlike most mainstream lossless compressors, Kanzi is not limited to a single compression paradigm. By combining multiple algorithms and techniques, it supports a broader range of compression ratios and adapts better to diverse data types.

Most traditional compressors underutilize modern hardware by running single-threaded—even on machines with many cores. Kanzi, in contrast, is concurrent by design, compressing multiple blocks in parallel across threads for significant performance gains. However, it is not compatible with standard compression formats.

It’s important to note that Kanzi is a data compressor, not an archiver. It includes optional checksums for verifying data integrity, but does not provide features like cross-file deduplication or data recovery mechanisms. That said, it produces a seekable bitstream, meaning one or more consecutive blocks can be decompressed independently, without needing to process the entire stream.

For more details, see [Wiki](https://github.com/flanglet/kanzi-cpp/wiki), [Q&A](https://github.com/flanglet/kanzi-cpp/wiki/q&a) and [DeepWiki](https://deepwiki.com/flanglet/kanzi-cpp/1-overview)

See how to reuse the C and C++ APIs: [here](https://github.com/flanglet/kanzi-cpp/wiki/Using-and-extending-the-code)

There is a Java implementation available here: https://github.com/flanglet/kanzi

There is a Go implementation available here: https://github.com/flanglet/kanzi-go

![Build Status](https://github.com/flanglet/kanzi-cpp/actions/workflows/c-cpp.yml/badge.svg)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=flanglet_kanzi-cpp&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=flanglet_kanzi-cpp)
[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=flanglet_kanzi-cpp&metric=ncloc)](https://sonarcloud.io/summary/new_code?id=flanglet_kanzi-cpp)
<a href="https://scan.coverity.com/projects/flanglet-kanzi-cpp">
  <img alt="Coverity Scan Build Status"
       src="https://img.shields.io/coverity/scan/16859.svg"/>
</a>
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/flanglet/kanzi-cpp)


## Why Kanzi

While excellent open-source compressors like zstd and brotli exist, they are primarily based on Lempel-Ziv (LZ) algorithms. Zstd, in particular, is a fantastic general-purpose choice known for its speed. However, LZ-based tools have inherent limits regarding compression ratios.

Kanzi offers a compelling alternative for specific high-performance scenarios:

* Beyond LZ: By incorporating Burrows-Wheeler Transform (BWT) and Context Modeling (CM), Kanzi can achieve compression ratios that traditional LZ methods cannot.

* Speed where it counts: While LZ is ideal for "compress once, decompress often" (like software distribution), it often slows down significantly at high compression settings. Kanzi leverages multi-core CPUs to maintain performance, making it highly effective for backups, real-time data generation, and one-off transfers.

* Content-Aware: Kanzi features built-in, customizable transforms for specific data types (e.g., multimedia, DNA, UTF text), improving efficiency where generic compressors fail.

* Extensible: The architecture is developer-friendly, making it straightforward to implement new transforms or entropy codecs for experimentation or niche data types.



## Benchmarks

Kanzi version 2.5.0 C++ implementation

_Note: The default block size at level 9 is 32MB. This limits the number of threads in use, especially with smaller files like enwik8, but all tests below are performed with default values._


### silesia.tar

Test machine:

AMD Ryzen 9 9950X 16-Core Processor running Ubuntu 25.10

Download at http://sun.aei.polsl.pl/~sdeor/corpus/silesia.zip

|        Compressor               |  Encoding (ms)  |  Decoding (ms)  |      Size        |
|---------------------------------|-----------------|-----------------|------------------|
|Original                         |                 |                 |   211,957,760    |
|lz4 1.1.10 -T16 -4               |        18       |         13      |    79,910,851    |
|**kanzi -l 1**                   |      **72**     |       **42**    |    79,331,051    |
|zstd 1.5.8 -T16 -2               |         6       |         11      |    69,443,247    |
|**kanzi -l 2**                   |      **64**     |       **42**    |    68,616,621    |
|brotli 1.1.0 -2                  |       880       |        333      |    68,040,160    |
|gzip 1.13 -9                     |     10328       |        704      |    67,651,076    |
|**kanzi -l 3**                   |     **109**     |       **58**    |    63,966,794    |
|zstd 1.5.8 -T16 -5               |       138       |        123      |    62,867,556    |
|**kanzi -l 4**                   |     **194**     |      **102**    |    61,183,757    |
|zstd 1.5.8 -T16 -9               |       320       |        114      |    59,233,481    |
|brotli 1.1.0 -6                  |      4039       |        299      |    58,511,709    |
|zstd 1.5.8 -T16 -13              |      1820       |        112      |    57,843,283    |
|brotli 1.1.0 -9                  |     23030       |        293      |    56,407,229    |
|bzip2 1.0.8 -9                   |      8223       |       3453      |    54,588,597    |
|**kanzi -l 5**                   |     **529**     |      **255**    |    53,853,702    |
|zstd 1.5.8 -T16 -19              |     11290       |        130      |    52,830,213    |
|**kanzi -l 6**                   |     **919**     |      **532**    |    49,472,084    |
|xz 5.8.1 -9                      |     43611       |        931      |    48,802,580    |
|bsc 3.3.11 -T16                  |      1201       |        698      |    47,900,848    |
|**kanzi -l 7**                   |    **1153**     |      **888**    |    47,330,422    |
|bzip3 1.5.1.r3-g428f422 -j 16    |      2348       |       2218      |    47,260,281    |
|**kanzi -l 8**                   |    **4473**     |     **4881**    |    42,962,913    |
|**kanzi -l 9**                   |   **11618**     |    **12381**    |    41,520,670    |




![Graph for Silesia on AMD Ryzen 9950X](doc/Plot_silesia.png)

Round-trip graph for Silesia on AMD Ryzen 9950X (X = compTime + 2*decompTime, Y = comp size)


### enwik8

Test machine:

Apple M3 24 GB macOS Sonoma 15.7.3

Download at https://mattmahoney.net/dc/enwik8.zip

|   Compressor    | Encoding (ms)  | Decoding (ms)  |    Size      |
|-----------------|----------------|----------------|--------------|
|Original         |                |                |  100,000,000 |
|kanzi -l 1       |       139      |         85     |   42,870,183 |
|kanzi -l 2       |       131      |         92     |   37,544,247 |
|kanzi -l 3       |       215      |        123     |   32,551,405 |
|kanzi -l 4       |       303      |        170     |   29,536,581 |
|kanzi -l 5       |       670      |        372     |   26,528,254 |
|kanzi -l 6       |      1009      |        727     |   24,076,765 |
|kanzi -l 7       |      1607      |       1366     |   22,817,360 |
|kanzi -l 8       |      6371      |       6752     |   21,181,992 |
|kanzi -l 9       |      8260      |       8760     |   20,035,144 |


![Graph for enwik8 on AMD Ryzen 9950X](doc/Plot_enwik8.png)

Round-trip graph for enwik8 on AMD Ryzen 9950X  (X = compTime + 2*decompTime, Y = comp size)



### More benchmarks

[Comprehensive lzbench benchmarks](https://github.com/flanglet/kanzi-cpp/wiki/Performance)

[More round trip scores](https://github.com/flanglet/kanzi-cpp/wiki/Round%E2%80%90trips-scores)


## Build Kanzi

* Platforms: Windows (Visual Studio), Linux, macOS, BSD
* Dependencies: None.
* Portability: Designed for easy porting to other OSs.
* Multithreading: Supported by default.

### Visual Studio 2008
Unzip the file "Kanzi_VS2008.zip" in place.
The solution generates a Windows 32 binary. Multithreading is not supported with this version.

### Visual Studio 2022
Unzip the file "Kanzi_VS2022.zip" in place.
The solution generates a Windows 64 binary and library.

### mingw-w64
Go to the source directory and run 'make clean && mingw32-make.exe kanzi'. The Makefile contains
all the necessary targets. Tested successfully on Win64 with mingw-w64 g++ 8.1.0.
Multithreading is supported with g++ version 5.0.0 or newer.
Builds successfully with C++11, C++14, C++17.

### Linux
Go to the source directory and run 'make clean && make kanzi'. The Makefile contains all the necessary
targets. Build successfully on Ubuntu with many versions of g++ and clang++.
Multithreading is supported with g++ version 5.0.0 or newer.
Builds successfully with C++98, C++11, C++14, C++17, C++20.

### macOS
Go to the source directory and run 'make clean && make kanzi'. The Makefile contains all the necessary
targets. Build successfully on MacOs with several versions of clang++.
Builds successfully with C++98, C++11, C++14, C++17, C++20.

### BSD
The makefile uses the gnu-make syntax. First, make sure gmake is present (or install it: 'pkg install gmake').
Go to the source directory and run 'gmake clean && gmake kanzi'. The Makefile contains all the necessary
targets. Builds successfully with C++98, C++11, C++14, C++17, C++20.

### Makefile targets
```
clean:          removes objects, libraries and binaries
kanzi:          builds the kanzi executable
kanzi_static:   builds a statically linked executable
kanzi_dynamic:  builds a dynamically linked executable
lib:            builds static and dynamic libraries
test:           builds test binaries
all:            kanzi + kanzi_static + kanzi_dynamic + lib + test
install:        installs libraries, headers and executable
uninstall:      removes installed libraries, headers and executable
```

For those who prefer cmake, run the following commands from the top directory:
```
mkdir build
cd build
cmake ..
make
ctest
```
By default, the cmake build generates a dynamically linked executable.
Choose ```make kanzi_static``` to build a statically linked executable.

Credits

Matt Mahoney,
Yann Collet,
Jan Ondrus,
Yuta Mori,
Ilya Muravyov,
Neal Burns,
Fabian Giesen,
Jarek Duda,
Ilya Grebnov

Disclaimer

Use at your own risk. Always keep a copy of your original files.
