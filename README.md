
# README

This is the Github Page of 7-Zip with support for **zstd**, short for
Zstandard, which is a fast lossless compression algorithm, targeting
real-time compression scenarios at zlib-level compression ratio.
LZ4 and LZ5 compression is also on the way, you can see some first
coding for it here: [Multithreading Library](https://github.com/mcmilk/zstdmt)

You can install it in two ways:

1. full setup with ZStandard additions within the GUI and an modified
   Explorer context menu
2. just the codec plugin, which goes to your existing 7-Zip installation

## 7-Zip ZStandard Edition (full setup, with GUI and Explorer integration)

### Installation (via setup)
1. download the setup from here [7-Zip ZS Releases](https://github.com/mcmilk/7-Zip-zstd/releases)
2. install it, like the default 7-Zip
3. use it ;)
4. you may check, if the 7-Zip can deal with ZStandard via this command: `7z.exe i`

The output should look like this:
```
7-Zip [64] 16.02 : Copyright (c) 1999-2016 Igor Pavlov : 2016-06-12


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
 0  ED  4F71101 ZSTD  <-- NEW
 0  ED  6F10701 7zAES
 0  ED  6F00181 AES256CBC
```

### Usage (full installation)

```
7z a archiv.7z -m0=zstd -mx0   Fastest Mode, without BCJ preprocessor
7z a archiv.7z -m0=zstd -mx1   Fast mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=zstd -mx..  ...
7z a archiv.7z -m0=zstd -mx21  2nd Slowest Mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=zstd -mx22  Ultra Mode, with BCJ preprocessor on executables

rem show contents of test.tar.zst:
7z x -so test.tar.zst | 7z l -si -ttar
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
7-Zip [64] 16.02 : Copyright (c) 1999-2016 Igor Pavlov : 2016-06-12


Libs:
 0  c:\Program Files\7-Zip\7z.dll
 1  c:\Program Files\7-Zip\Codecs\zstd-x64.dll

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
 1  ED  4F71101 ZSTD  <-- NEW
```

### Usage (codec plugin)

- when compressing binaries (*.exe, *.dll), you have to explicitly disable
  the bcj2 filter via `-m0=bcj`, when using only the plugin dll's
- so the usage should look like this:
```
7z a archiv.7z -m0=bcj -m1=zstd -mx1   Fast mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=bcj -m1=zstd -mx..  ...
7z a archiv.7z -m0=bcj -m1=zstd -mx21  2nd Slowest Mode, with BCJ preprocessor on executables
7z a archiv.7z -m0=bcj -m1=zstd -mx22  Ultra Mode, with BCJ preprocessor on executables
```
- you can only create .7z files with zstd compression, but you can not create .zst files :/

## Links
- [ZStandard Homepage](https://github.com/Cyan4973/zstd)
- [7-Zip ZStandard Homepage](https://mcmilk.de/projects/7-Zip-zstd/)
- Request for inclusion into the mainline 7-Zip: https://sourceforge.net/p/sevenzip/discussion/45797/thread/a7e4f3f3/
  - result, will not be included :(
- [Support me](https://www.paypal.me/TinoReichardt) - when you want

## Benchmarks with i7-3632QM
![Compression Speed vs Ratio](https://mcmilk.de/projects/7-Zip-zstd/dl/compr-074-usb2.png "Compression Speed vs Ratio")
![Decompression Speed](https://mcmilk.de/projects/7-Zip-zstd/dl/decompr-074.png "Decompression Speed @ Windows 7 64Bit")

## License and redistribution

- the same as the original 7-Zip, which means GNU GPL


/TR 2016-09-15 (ZStandard Version 1.0.1, 7-Zip Version 16.02)
