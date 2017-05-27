
# README

This is the Github Page of 7-Zip ZS with support of additional Codecs. The library used therefore is located here: [Multithreading Library](https://github.com/mcmilk/zstdmt)

You can install it in two ways:
1. full setup with additions within the GUI and an modified an Explorer context menu
2. just the codec plugin, which goes to your existing 7-Zip installation

## Codec overview
1. [Zstandard] is a real-time compression algorithm, providing high compression ratios. It offers a very wide range of compression / speed trade-off, while being backed by a very fast decoder.
   - Levels: 1..22

2. [Brotli] is a generic-purpose lossless compression algorithm that compresses data using a combination of a modern variant of the LZ77 algorithm, Huffman coding and 2nd order context modeling, with a compression ratio comparable to the best currently available general-purpose compression methods. It is similar in speed with deflate but offers more dense compression.
   - Levels: 0..11

3. [LZ4] is lossless compression algorithm, providing compression speed at 400 MB/s per core (0.16 Bytes/cycle). It features an extremely fast decoder, with speed in multiple GB/s per core (0.71 Bytes/cycle). A high compression derivative, called LZ4_HC, is available, trading customizable CPU time for compression ratio.
   - Levels: 1..12

4. [LZ5] is a modification of LZ4 which gives a better ratio at cost of slower compression and decompression.
   - Levels: 1..15

5. [Lizard] is an efficient compressor with very fast decompression. It achieves compression ratio that is comparable to zip/zlib and zstd/brotli (at low and medium compression levels) at decompression speed of 1000 MB/s and faster.
   - Levels: 10..49 (10..19 for method1, 20..29 for method2, ...)

## 7-Zip ZStandard Edition (full setup, with GUI and Explorer integration)

### Installation (via setup)
1. download the setup from here [7-Zip ZS Releases](https://github.com/mcmilk/7-Zip-zstd/releases)
2. install it, like the default [7-Zip]
3. use it ;)
4. you may check, if the 7-Zip can deal with [ZStandard] or other codecs via this command: `7z.exe i`

The output should look like this:
```
7-Zip 17.00 ZS v1.2.0 R3 (x64) : Copyright (c) 1999-2017 Igor Pavlov, 2016-2017 Tino Reichardt : 2017-05-25


Libs:
 0  c:\Program Files\7-Zip-ZStandard\7z.dll

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
 0  ED  6F10701 7zAES
 0  ED  6F00181 AES256CBC
```

### Usage and features of the full installation

- compression and decompression for [Brotli], [Lizard], [LZ4], [LZ5] and [ZStandard] within the 7-Zip container format
- compression and decompression of [LZ4] (`.lz4`), [LZ5] (`.lz5`) and [ZStandard] (`.zst`) files
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

7z x -so test.tar.zstd | 7z l -si -ttar
-> show contents of zstd compressed tar archiv test.tar.zstd

7z x -so test.tar.lz | 7z l -si -ttar
-> show contents of lzip compressed tar archiv test.tar.lz
```

![Explorer inegration](https://mcmilk.de/projects/7-Zip-zstd/Add-To-Archive.png "Add to Archiv Dialog with ZSTD options")
![File Manager](https://mcmilk.de/projects/7-Zip-zstd/Fileman.png "File Manager with the Listing of an Archiv")

## ZStandard codec Plugin for 7-Zip

### Installation (via plugin)

1. download the codec archiv from here [7-Zip ZS Releases](https://github.com/mcmilk/7-Zip-zstd/releases)
2. create a new directory named "Codecs"
3. put in there the zstd-x32.dll or the zstd-x64.dll, depending on your 7-Zip installation
   - normally, the x32 should go to: "C:\Program Files (x86)\7-Zip\Codecs"
   - the x64 version should go in here: "C:\Program Files\7-Zip\Codecs"
4. After this, you may check if the dll is correctly installed via this command: `7z.exe i`

The output should look like this:
```
7-Zip 17.00 beta (x64) : Copyright (c) 1999-2017 Igor Pavlov : 2017-04-29


Libs:
 0  c:\Program Files\7-Zip\7z.dll
 1  c:\Program Files\7-Zip\Codecs\brotli-x64.dll
 2  c:\Program Files\7-Zip\Codecs\lizard-x64.dll
 3  c:\Program Files\7-Zip\Codecs\lz4-x64.dll
 4  c:\Program Files\7-Zip\Codecs\lz5-x64.dll
 5  c:\Program Files\7-Zip\Codecs\zstd-x64.dll

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
 2  ED  4F71106 LIZARD
 3  ED  4F71104 LZ4
 4  ED  4F71105 LZ5
 5  ED  4F71101 ZSTD
```

### Usage (codec plugin)

- compression and decompression for [Brotli], [Lizard], [LZ4], [LZ5] and [ZStandard] within the 7-Zip container format
- you can only create `.7z` files, the files like `.lz4`, `.lz5` and `.zst` are not covered by the plugins
- when compressing binaries (*.exe, *.dll), you have to explicitly disable the bcj2 filter via `-m0=bcj`,
  when using only the plugin dll's
- so the usage should look like this:
```
7z a archiv.7z -m0=bcj -m1=zstd -mx1   Fast mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=bcj -m1=zstd -mx..  ...
7z a archiv.7z -m0=bcj -m1=zstd -mx21  2nd Slowest Mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=bcj -m1=zstd -mx22  Ultra Mode, with BCJ preprocessor on executables
```
## Benchmarks with i7-3632QM
![Compression Speed vs Ratio](https://mcmilk.de/projects/7-Zip-zstd/dl/7z1700_v120_ratio.png "Compression Speed vs Ratio")
![Decompression Speed](https://mcmilk.de/projects/7-Zip-zstd/dl/decompr-074.png "Decompression Speed @ Windows 7 64Bit")

## License and redistribution

- the same as the original 7-Zip, which means GNU GPL

## Links

- [7-Zip Homepage](http://www.7-zip.org/)
- [7-Zip ZStandard Homepage](https://mcmilk.de/projects/7-Zip-zstd/)
- Request for inclusion into the mainline 7-Zip: https://sourceforge.net/p/sevenzip/discussion/45797/thread/a7e4f3f3/
  - result, will not be included :(

## Donate
If you find this project useful, you can...

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.me/TinoReichardt)

## Version Information

- 7-Zip ZS Version 17.00
  - [Brotli] Version 0.6.0
  - [Lizard] Version 2.0
  - [LZ4] Version 1.7.5
  - [LZ5] Version 1.5
  - [ZStandard] Version 1.2.0

/TR 2017-05-25

[7-Zip]:http://www.7-zip.org/
[lzip]:http://www.nongnu.org/lzip/
[Brotli]:https://github.com/google/brotli/
[LZ4]:https://github.com/lz4/lz4/
[LZ5]:https://github.com/inikep/lz5/
[ZStandard]:https://github.com/facebook/zstd/
[Lizard]:https://github.com/inikep/lizard/
