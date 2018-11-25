
# README

This is the Github Page of [7-Zip] ZS with support of additional Codecs. The library used therefore is located here: [Multithreading Library](https://github.com/mcmilk/zstdmt)

You can install it in two ways:
1. complete setup with additions within the GUI and a modified Explorer context menu
2. only the codec plugin that goes to your existing [7-Zip] installation

# Status

[![Build status](https://ci.appveyor.com/api/projects/status/j9cwlxqe1g21c4dj?svg=true)](https://ci.appveyor.com/project/mcmilk/7-zip-zstd)
[![Latest stable release](https://img.shields.io/github/release/mcmilk/7-Zip-zstd.svg)](https://github.com/mcmilk/7-Zip-zstd/releases)
[![PayPal.me](https://img.shields.io/badge/PayPal-me-blue.svg?maxAge=2592000)](https://www.paypal.me/TinoReichardt)


## Codec overview
1. [Zstandard] v1.3.7 is a real-time compression algorithm, providing high compression ratios. It offers a very wide range of compression / speed trade-off, while being backed by a very fast decoder.
   - Levels: 1..22

2. [Brotli] v.1.0.7 is a generic-purpose lossless compression algorithm that compresses data using a combination of a modern variant of the LZ77 algorithm, Huffman coding and 2nd order context modeling, with a compression ratio comparable to the best currently available general-purpose compression methods. It is similar in speed with deflate but offers more dense compression.
   - Levels: 0..11

3. [LZ4] v1.8.3 is lossless compression algorithm, providing compression speed at 400 MB/s per core (0.16 Bytes/cycle). It features an extremely fast decoder, with speed in multiple GB/s per core (0.71 Bytes/cycle). A high compression derivative, called LZ4_HC, is available, trading customizable CPU time for compression ratio.
   - Levels: 1..12

4. [LZ5] v1.5 is a modification of LZ4 which gives a better ratio at cost of slower compression and decompression.
   - Levels: 1..15

5. [Lizard] v1.0 is an efficient compressor with very fast decompression. It achieves compression ratio that is comparable to zip/zlib and zstd/brotli (at low and medium compression levels) at decompression speed of 1000 MB/s and faster.
   - Levels 10..19 (fastLZ4) are designed to give about 10% better decompression speed than LZ4
   - Levels 20..29 (LIZv1) are designed to give better ratio than LZ4 keeping 75% decompression speed
   - Levels 30..39 (fastLZ4 + Huffman) adds Huffman coding to fastLZ4
   - Levels 40..49 (LIZv1 + Huffman) give the best ratio, comparable to zlib and low levels of zstd/brotli, but with a faster decompression speed

6. [Fast LZMA2] v0.9.2 is a LZMA2 compression algorithm, 20% to 100% faster than normal LZMA2 at levels 5 and above, but with a slightly lower compression ratio. It uses a parallel buffered radix matchfinder and some optimizations from Zstandard. The codec uses much less additional memory per thread than standard LZMA2.
   - Levels: 1..9

## 7-Zip Zstandard Edition (full setup, with GUI and Explorer integration)

### Installation (via setup)
1. download the setup from here [7-Zip ZS Releases](https://github.com/mcmilk/7-Zip-zstd/releases)
2. install it, like the default [7-Zip]
3. use it ;)
4. you may check, if the [7-Zip] can deal with [Zstandard] or other codecs via this command: `7z.exe i`
5. the binaries within this installation are not binary compatible with [7-Zip]... use therefore the files from the `Codecs.7z` archive

The output should look like this:
```
7-Zip 18.05 ZS v1.3.7 R1 (x64) : Copyright (c) 1999-2017 Igor Pavlov : 2018-10-21


Libs:
 0  c:\Program Files\7-Zip-Zstandard\7z.dll

Formats:
...
 0 CK            xz       xz txz (.tar) FD 7 z X Z 00
 0               Z        z taz (.tar)  1F 9D
 0 CK            zstd     zst tzstd (.tar) 0 x F D 2 F B 5 2 5 . . 0 x F D 2 F B 5 2 8 00
 0 C   F         7z       7z            7 z BC AF ' 1C
 0     F         Cab      cab           M S C F 00 00 00 00
...

Codecs:
 0 4ED  303011B BCJ2
 0  ED  3030103 BCJ
 0  ED  3030205 PPC
 0  ED  3030401 IA64
 0  ED  3030501 ARM
 0  ED  3030701 ARMT
 0  ED  3030805 SPARC
 0  ED    20302 Swap2
 0  ED    20304 Swap4
 0  ED    40202 BZip2
 0  ED        0 Copy
 0  ED    40109 Deflate64
 0  ED    40108 Deflate
 0  ED        3 Delta
 0  ED       21 LZMA2
 0  ED    30101 LZMA
 0  ED    30401 PPMD
 0   D    40301 Rar1
 0   D    40302 Rar2
 0   D    40303 Rar3
 0   D    40305 Rar5
 0  ED  4F71102 BROTLI
 0  ED  4F71104 LZ4
 0  ED  4F71106 LIZARD
 0  ED  4F71105 LZ5
 0  ED  4F71101 ZSTD
 0  ED       21 FLZMA2
 0  ED  6F10701 7zAES
 0  ED  6F00181 AES256CBC
```

### Usage and features of the full installation

- compression and decompression for [Brotli], [Lizard], [LZ4], [LZ5] and [Zstandard] within the [7-Zip] container format
- compression and decompression of [Lizard] (`.liz`), [LZ4] (`.lz4`), [LZ5] (`.lz5`) and [Zstandard] (`.zst`) files
- included [lzip] decompression support, patch from: http://download.savannah.gnu.org/releases/lzip/7zip/
- right click and _"Add to xy.7z"_ will use the last selected method (codec, level and threads)
- the FileManager ListBox will show more information about these codecs now

```
7z a archiv.7z -m0=zstd -mx0   Zstandard Fastest Mode, without BCJ preprocessor
7z a archiv.7z -m0=zstd -mx1   Zstandard Fast mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=zstd -mx..  ...
7z a archiv.7z -m0=zstd -mx21  Zstandard 2nd Slowest Mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=zstd -mx22  Zstandard Ultra Mode, with BCJ preprocessor on executables

7z a archiv.7z -m0=lz4 -mx0   LZ4 Fastest Mode, without BCJ preprocessor
7z a archiv.7z -m0=lz4 -mx1   LZ4 Fast mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=lz4 -mx..  ...
7z a archiv.7z -m0=lz4 -mx12  LZ4 Ultra Mode, with BCJ preprocessor on executables

7z a archiv.7z -m0=lz5 -mx0   LZ5 Version 1.5 Fastest Mode, without BCJ preprocessor
7z a archiv.7z -m0=lz5 -mx1   LZ5 Version 1.5 Fast mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=lz5 -mx..  ...
7z a archiv.7z -m0=lz5 -mx16  LZ5 Version 1.5 Ultra Mode, with BCJ preprocessor on executables

7z a archiv.7z -m0=flzma2 -mx1   Fast LZMA2 Fastest mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=flzma2 -mx..  ...
7z a archiv.7z -m0=flzma2 -mx9   Fast LZMA2 Ultra Mode, with BCJ preprocessor on executables

7z x -so test.tar.zstd | 7z l -si -ttar
-> show contents of zstd compressed tar archiv test.tar.zstd

7z x -so test.tar.lz | 7z l -si -ttar
-> show contents of lzip compressed tar archiv test.tar.lz
```

![Explorer inegration](https://mcmilk.de/projects/7-Zip-zstd/Add-To-Archive.png "Add to Archiv Dialog with ZSTD options")
![File Manager](https://mcmilk.de/projects/7-Zip-zstd/Fileman.png "File Manager with the Listing of an Archiv")
![Methods](https://mcmilk.de/projects/7-Zip-zstd/Methods2.png "Methods")
![Hashes](https://mcmilk.de/projects/7-Zip-zstd/Hashes.png "Hashes")

## Zstandard codec Plugin for Mainline 7-Zip

### Installation (via plugin)

1. download the `Codecs.7z` archiv from here [7-Zip ZS Releases](https://github.com/mcmilk/7-Zip-zstd/releases), this archive holds binaries, which are compatible with the Mainline version of [7-Zip]
2. create a new directory named `Codecs` and put in there the zstd-x32.dll or the zstd-x64.dll, depending on your [7-Zip] installation
   - normally, the x32 should go to: "C:\Program Files (x86)\7-Zip\Codecs"
   - the x64 version should go in here: "C:\Program Files\7-Zip\Codecs"
3. you could also replace the `7z.dll` directly within `C:\Program Files (x86)\7-Zip`
4. then you may check if the dll is correctly installed via this command: `7z.exe i`

The output should look like this:
```
7-Zip 18.05 (x64) : Copyright (c) 1999-2018 Igor Pavlov : 2018-04-30

Libs:
 0  C:\Program Files\7-Zip\7z.dll

Libs:
 0  c:\Program Files\7-Zip\7z.dll
 1  c:\Program Files\7-Zip\Codecs\brotli-x64.dll
 2  c:\Program Files\7-Zip\Codecs\flzma2-x64.dll
 3  c:\Program Files\7-Zip\Codecs\lizard-x64.dll
 4  c:\Program Files\7-Zip\Codecs\lz4-x64.dll
 5  c:\Program Files\7-Zip\Codecs\lz5-x64.dll
 6  c:\Program Files\7-Zip\Codecs\zstd-x64.dll

...

Codecs:
 0 4ED  303011B BCJ2
 0  ED  3030103 BCJ
 0  ED  3030205 PPC
 0  ED  3030401 IA64
 0  ED  3030501 ARM
 0  ED  3030701 ARMT
 0  ED  3030805 SPARC
 0  ED    20302 Swap2
 0  ED    20304 Swap4
 0  ED    40202 BZip2
 0  ED        0 Copy
 0  ED    40109 Deflate64
 0  ED    40108 Deflate
 0  ED        3 Delta
 0  ED       21 LZMA2
 0  ED    30101 LZMA
 0  ED    30401 PPMD
 0   D    40301 Rar1
 0   D    40302 Rar2
 0   D    40303 Rar3
 0   D    40305 Rar5
 0  ED  6F10701 7zAES
 0  ED  6F00181 AES256CBC
 1  ED  4F71102 BROTLI
 2  ED       21 FLZMA2
 3  ED  4F71106 LIZARD
 4  ED  4F71104 LZ4
 5  ED  4F71105 LZ5
 6  ED  4F71101 ZSTD
```

### Usage (codec plugin)

- compression and decompression for [Brotli], [Lizard], [LZ4], [LZ5] and [Zstandard] within the 7-Zip container format
- you can only create `.7z` files, the files like `.lz4`, `.lz5` and `.zst` are not covered by the plugins
- when compressing binaries (*.exe, *.dll), you have to explicitly disable the bcj2 filter via `-m0=bcj`,
  when using only the plugin dll's
- so the usage should look like this:
```
7z a archiv.7z -m0=bcj -m1=zstd -mx1   Fast mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=bcj -m1=zstd -mx..  ...
7z a archiv.7z -m0=bcj -m1=zstd -mx21  2nd Slowest Mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=bcj -m1=zstd -mx22  Ultra Mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=bcj -m1=brotli -mxN  ...
7z a archiv.7z -m0=bcj -m1=lizard -mxN  ...
7z a archiv.7z -m0=bcj -m1=lz4 -mxN  ...
7z a archiv.7z -m0=bcj -m1=lz5 -mxN  ...
7z a archiv.7z -m0=bcj -m1=flzma2 -mxN  ...
```

## Codec Plugin for Total Commander
- download [TotalCmd.7z]
- install it, by replacing the files `tc7z.dll` and `tc7z64.dll` with the new ones
- you can check the Total Commander Forum for more information about this [DLL Files](http://ghisler.ch/board/viewtopic.php?p=319216)
- decompression for [Brotli], [Lizard], [LZ4], [LZ5] and [Zstandard] ot the 7-Zip `.7z` format
  will work out of the box with Total Commander now :-)

## Codec Plugin for Far Manager
- download [Codecs.7z]
- install it, by replacing the file `C:\Program Files\Far Manager\Plugins\ArcLite\7z.dll` with the one found in the [Codecs.7z] archive
- it's named `7z-x64.dll` or `7z-x32.dll`, depending on your architecture
- then restart the Far manager - and on next start, you will have support for 7-Zip Zstandard archives ;-)

## Benchmarks

For the benchmarks I am using Windows 7 64bit on my Laptop which has the following Hardware:
- Intel i7-3632QM, limited to 2,2GHz, 16GB RAM, disabled swap space
- the compression benchmark is read / written to an 4 GiB [ImDisk]
- the decompression benchmark is also done in RAM via: `7z t archiv.7z`
- the tool for measuring the times is [wtime](https://github.com/mcmilk/wtime), together with some [scripts](https://github.com/mcmilk/7-Zip-Benchmarking)
- the testfile is generated via [generate-mcorpus](https://github.com/mcmilk/7-Zip-Benchmarking/blob/master/generate-mcorpus)
![Compression Speed vs Ratio](https://mcmilk.de/projects/7-Zip-zstd/dl/compr-v120.png "Compression Speed vs Ratio")
![Decompression Speed](https://mcmilk.de/projects/7-Zip-zstd/dl/decomp-v120.png "Decompression Speed per Level")
![Memory at Compression](https://mcmilk.de/projects/7-Zip-zstd/dl/MemCompr.png "Memory usage at Compression")
![Memory at Decompression](https://mcmilk.de/projects/7-Zip-zstd/dl/MemDecomp.png "Memory usage at Decompression")

## License and redistribution

- the same as the Mainline [7-Zip], which means GNU LGPL

## Links

- [7-Zip Homepage](http://www.7-zip.org/)
- [7-Zip Zstandard Homepage](https://mcmilk.de/projects/7-Zip-zstd/)
- [Request for inclusion](https://sourceforge.net/p/sevenzip/discussion/45797/thread/a7e4f3f3/) into the mainline 7-Zip:
  - result, will currently not included :(

## Donate

You find this project useful, maybe you consider a donation ;-)

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.me/TinoReichardt)

## Version Information

- 7-Zip ZS Version 18.05
  - [Brotli] Version 1.0.7
  - [Lizard] Version 1.0
  - [LZ4] Version 1.8.3
  - [LZ5] Version 1.5
  - [Zstandard] Version 1.3.7
  - [Fast LZMA2] Version 0.9.2

/TR 2018-11-25

## Notes

- if you want an code signed installer, you need to donate sth.
- I know that Zstandard has some nice new features which are not included currently... I will think about it

[7-Zip]:http://www.7-zip.org/
[lzip]:http://www.nongnu.org/lzip/
[Brotli]:https://github.com/google/brotli/
[LZ4]:https://github.com/lz4/lz4/
[LZ5]:https://github.com/inikep/lz5/
[Zstandard]:https://github.com/facebook/zstd/
[Lizard]:https://github.com/inikep/lizard/
[ImDisk]:https://sourceforge.net/projects/imdisk-toolkit/
[Fast LZMA2]:https://github.com/conor42/fast-lzma2

[Codecs.7z]:https://github.com/mcmilk/7-Zip-zstd/releases
[TotalCmd.7z]:https://github.com/mcmilk/7-Zip-zstd/releases
