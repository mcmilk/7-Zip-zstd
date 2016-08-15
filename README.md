
This is the Github Page of 7-Zip with support for **zstd**, short for
Zstandard, which is a fast lossless compression algorithm, targeting
real-time compression scenarios at zlib-level compression ratio.

You can install it in two ways:
1. A full setup with ZStandard additions within the GUI and an modified
   Explorer context menu
2. just the codec plugin, which goes to your existing 7-Zip installation


![Compression Speed vs Ratio](https://mcmilk.de/projects/7-Zip-zstd/dl/compr-074-usb2.png "Compression Speed vs Ratio")
![Decompression Speed](https://mcmilk.de/projects/7-Zip-zstd/dl/decompr-074.png "Decompression Speed @ Windows 7 64Bit")

# 7-Zip ZStandard Edition (full setup, with GUI and Explorer integration)

## Installation

1. download the setup from here https://mcmilk.de/projects/7-Zip-zstd/dl/
   - https://mcmilk.de/projects/7-Zip-zstd/dl/7z1602-zstd-x32.exe
   - https://mcmilk.de/projects/7-Zip-zstd/dl/7z1602-zstd-x64.exe
2. install it

## Usage

![File Manager](https://mcmilk.de/projects/7-Zip-zstd/Fileman.png "File Manager with the Listing of an Archiv")
![Explorer inegration](https://mcmilk.de/projects/7-Zip-zstd/Add-To-Archive.png "Add to Archiv Dialog with ZSTD options")

# ZStandard codec Plugin for 7-Zip

## Installation

1. download the codec archiv from https://mcmilk.de/projects/7-Zip-zstd/dl/Codecs.7z
2. create a new directory named "Codecs"
3. put in there the zstd-x32.dll or the zstd-x64.dll, depending on your 7-Zip installation
   - normally, the x32 should go to: "C:\Program Files (x86)\7-Zip\Codecs"
   - and the x64 version should go in here: "C:\Program Files\7-Zip\Codecs"
4. After this, you may check if the dll is correctly installed via this command: `7z.exe i`

The output should look like this:
```
Codecs:
 0  ED    40202 BZip2
 0 4ED  303011B BCJ2
 0  ED  3030103 BCJ
 0  ED  3030205 PPC
 0  ED  3030401 IA64
 0  ED  3030501 ARM
 0  ED  3030701 ARMT
 0  ED  3030805 SPARC
 0  ED    20302 Swap2
 0  ED    20304 Swap4
 0  ED        0 Copy
 0  ED    40109 Deflate64
 0  ED    40108 Deflate
 0  ED        3 Delta
 0  ED       21 LZMA2
 0  ED    30101 LZMA
 0  ED    30401 PPMD
 0  ED  6F10701 7zAES
 0  ED  6F00181 AES256CBC
 1   D    40301 Rar1
 1   D    40302 Rar2
 1   D    40303 Rar3
 1   D    40305 Rar5
 1  ED  4F71101 ZSTD  <-- NEW
```

## Usage

- when compressing binaries (*.exe, *.dll), you have to explicitly disable
  the bcj2 filter via `-m0=bcj`

- so the usage should look like this:
```
7z a archiv.7z -m0=bcj -m1=zstd -mx0   ...Fastest Mode, without BCJ preprocessor
7z a archiv.7z -m0=bcj -m1=zstd -mx1   ...Fast mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=bcj -m1=zstd -mx..  ...
7z a archiv.7z -m0=bcj -m1=zstd -mx21  ...2nd Slowest Mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=bcj -m1=zstd -mx22  ...Ultra Mode, with BCJ preprocessor on executables
```

# Links
- ZStandard Homepage: https://github.com/Cyan4973/zstd
- 7-Zip ZStandard Homepage: https://mcmilk.de/projects/7-Zip-zstd/dl/
- Request for inclusion into the mainline 7-Zip: https://sourceforge.net/p/sevenzip/discussion/45797/thread/a7e4f3f3/
  - result, will not be included :(

/TR 2016-08-15
